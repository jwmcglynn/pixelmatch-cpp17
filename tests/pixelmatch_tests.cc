#include <gmock/gmock.h>
#include <gtest/gtest-death-test.h>
#include <gtest/gtest.h>

#include <array>
#include <filesystem>

#include "pixelmatch/image_utils.h"
#include "pixelmatch/pixelmatch.h"

namespace pixelmatch {

std::ostream& operator<<(std::ostream& os, const Color& color) {
  return os << "rgba(" << static_cast<int>(color.r) << " " << static_cast<int>(color.g) << " "
            << static_cast<int>(color.b) << " " << static_cast<int>(color.a) << ")";
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::optional<T>& value) {
  if (value) {
    return os << *value;
  } else {
    return os << "nullopt";
  }
}

std::ostream& operator<<(std::ostream& os, const Options& options) {
  return os << "Options{threshold=" << options.threshold << ", includeAA=" << options.includeAA
            << ", alpha=" << options.alpha << ", aaColor=" << options.aaColor
            << ", diffColor=" << options.diffColor << ", diffColorAlt=" << options.diffColorAlt
            << ", diffMask=" << options.diffMask << "}";
}

std::string escapeFilename(std::string filename) {
  std::transform(filename.begin(), filename.end(), filename.begin(), [](char c) {
    if (c == '\\' || c == '/') {
      return '_';
    } else {
      return c;
    }
  });
  return filename;
}

void diffTest(const char* filename1, const char* filename2, const char* diffFilename,
              Options options, int expectedMismatch) {
  SCOPED_TRACE(testing::Message() << "Comparing " << filename1 << " to " << filename2 << ", "
                                  << options);

  auto maybeImg1 = readRgbaImageFromPngFile(filename1);
  ASSERT_TRUE(maybeImg1.has_value()) << "Failed to load filename1: " << filename1;
  const Image img1 = std::move(maybeImg1.value());

  auto maybeImg2 = readRgbaImageFromPngFile(filename2);
  ASSERT_TRUE(maybeImg2.has_value()) << "Failed to load filename2: " << filename2;
  const Image img2 = std::move(maybeImg2.value());

  ASSERT_EQ(img1.width, img2.width)
      << "Size mismatch between " << filename1 << " and " << filename2;
  ASSERT_EQ(img1.height, img2.height)
      << "Size mismatch between " << filename1 << " and " << filename2;
  ASSERT_EQ(img1.strideInPixels, img2.strideInPixels)
      << "Stride mismatch between " << filename1 << " and " << filename2;

  std::vector<uint8_t> diff;
  diff.resize(img1.strideInPixels * img1.height * 4);

  const int mismatch =
      pixelmatch(img1.data, img2.data, diff, img1.width, img1.height, img1.strideInPixels, options);
  const int mismatchWithoutDiff = pixelmatch(img1.data, img2.data, span<uint8_t>(), img1.width,
                                             img1.height, img1.strideInPixels, options);

  if (std::getenv("UPDATE_TEST_IMAGES") != nullptr) {
    writeRgbaPixelsToPngFile(diffFilename, diff, img1.width, img1.height, img1.strideInPixels);
  } else {
    auto maybeExpectedDiff = readRgbaImageFromPngFile(diffFilename);
    ASSERT_TRUE(maybeExpectedDiff.has_value()) << "Failed to load diffFilename: " << diffFilename;
    const Image expectedDiff = std::move(maybeExpectedDiff.value());

    ASSERT_EQ(img1.width, expectedDiff.width)
        << "Size mismatch between " << filename1 << " and " << diffFilename;
    ASSERT_EQ(img1.height, expectedDiff.height)
        << "Size mismatch between " << filename1 << " and " << diffFilename;
    ASSERT_EQ(img1.strideInPixels, expectedDiff.strideInPixels)
        << "Stride mismatch between " << filename1 << " and " << diffFilename;

    const bool diffEqualsExpected = imageEquals(diff, expectedDiff.data, expectedDiff.width,
                                                expectedDiff.height, expectedDiff.strideInPixels);
    if (!diffEqualsExpected) {
      std::filesystem::path actualDiffFilename =
          std::filesystem::temp_directory_path() / escapeFilename(diffFilename);
      std::cerr << "Saving actual diff to: " << actualDiffFilename << std::endl;

      writeRgbaPixelsToPngFile(actualDiffFilename.c_str(), diff, img1.width, img1.height,
                               img1.strideInPixels);
    }
    EXPECT_TRUE(diffEqualsExpected)
        << "Computed image diff and expected version in " << diffFilename << " do not match";
  }

  EXPECT_EQ(mismatch, expectedMismatch) << "Different number of mismatched pixels";
  EXPECT_EQ(mismatch, mismatchWithoutDiff)
      << "Mismatched pixels differ when diff output is disabled";
}

/**
 * Return true if the two pixels are identical.
 */
bool compareSinglePixel(const Color& pixel1, const Color& pixel2) {
  constexpr int width = 1;
  constexpr int height = 1;
  constexpr size_t strideInPixels = 1;

  constexpr size_t bytes = strideInPixels * height * 4;

  std::array<uint8_t, bytes> img1{};
  std::array<uint8_t, bytes> img2{};
  std::array<uint8_t, bytes> output{};

  img1[0] = pixel1.r;
  img1[1] = pixel1.g;
  img1[2] = pixel1.b;
  img1[3] = pixel1.a;

  img2[0] = pixel2.r;
  img2[1] = pixel2.g;
  img2[2] = pixel2.b;
  img2[3] = pixel2.a;

  Options options;
  options.includeAA = true;
  options.threshold = 0;

  const int difference = pixelmatch(img1, img2, output, width, height, strideInPixels, options);
  EXPECT_NE(difference, -1) << "Preconditions failed";
  return difference == 0;
}

Options defaultTestOptions() {
  Options result;
  result.threshold = 0.05f;
  return result;
}

TEST(Pixelmatch, Validate1Diff) {
  diffTest("tests/testdata/1a.png", "tests/testdata/1b.png", "tests/testdata/1diff.png",
           defaultTestOptions(), 143);
}

TEST(Pixelmatch, Validate1DiffMask) {
  Options options;
  options.threshold = 0.05f;
  options.includeAA = false;
  options.diffMask = true;

  diffTest("tests/testdata/1a.png", "tests/testdata/1b.png", "tests/testdata/1diffmask.png",
           options, 143);
}

TEST(Pixelmatch, Validate1EmptyDiffMask) {
  Options options;
  options.threshold = 0.0f;
  options.diffMask = true;

  diffTest("tests/testdata/1a.png", "tests/testdata/1a.png", "tests/testdata/1emptydiffmask.png",
           options, 0);
}

TEST(Pixelmatch, Validate2Diff) {
  Options options;
  options.threshold = 0.05f;
  options.alpha = 0.5f;
  options.aaColor = Color{0, 192, 0, 255};
  options.diffColor = Color{255, 0, 255, 255};

  diffTest("tests/testdata/2a.png", "tests/testdata/2b.png", "tests/testdata/2diff.png", options,
           12437);
}

TEST(Pixelmatch, Validate3Diff) {
  diffTest("tests/testdata/3a.png", "tests/testdata/3b.png", "tests/testdata/3diff.png",
           defaultTestOptions(), 212);
}

TEST(Pixelmatch, Validate4Diff) {
  diffTest("tests/testdata/4a.png", "tests/testdata/4b.png", "tests/testdata/4diff.png",
           defaultTestOptions(), 36049);
}

TEST(Pixelmatch, Validate5Diff) {
  diffTest("tests/testdata/5a.png", "tests/testdata/5b.png", "tests/testdata/5diff.png",
           defaultTestOptions(), 0);
}

TEST(Pixelmatch, Validate6Diff) {
  diffTest("tests/testdata/6a.png", "tests/testdata/6b.png", "tests/testdata/6diff.png",
           defaultTestOptions(), 51);
}

TEST(Pixelmatch, Validate6Empty) {
  Options options;
  options.threshold = 0.0f;

  diffTest("tests/testdata/6a.png", "tests/testdata/6a.png", "tests/testdata/6empty.png", options,
           0);
}

TEST(Pixelmatch, Validate6IncludeAA) {
  Options options;
  options.threshold = 0.05;
  options.includeAA = true;

  diffTest("tests/testdata/6a.png", "tests/testdata/6b.png", "tests/testdata/6diffaa.png", options,
           4900);
}

TEST(Pixelmatch, Validate7Diff) {
  Options options;
  options.diffColorAlt = Color{0, 255, 0, 255};

  diffTest("tests/testdata/7a.png", "tests/testdata/7b.png", "tests/testdata/7diff.png", options,
           2448);
}

TEST(Pixelmatch, WithStride) {
  constexpr int width = 3;
  constexpr int height = 4;
  constexpr size_t stride = 4;

  constexpr size_t bytes = stride * height * 4;

  std::array<uint8_t, bytes> img1{};
  std::array<uint8_t, bytes> img2{};
  std::array<uint8_t, bytes> output{};

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      img1[(y * stride + x) * 4 + 0] = 128;
      img1[(y * stride + x) * 4 + 1] = 128;
      img1[(y * stride + x) * 4 + 2] = 128;
      img1[(y * stride + x) * 4 + 3] = 255;
    }
  }

  Options options;
  options.includeAA = true;

  EXPECT_EQ(pixelmatch(img1, img2, output, width, height, stride, options), 12);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      EXPECT_EQ(output[(y * stride + x) * 4 + 0], 255) << "x=" << x << ", y=" << y;
      EXPECT_EQ(output[(y * stride + x) * 4 + 1], 0) << "x=" << x << ", y=" << y;
      EXPECT_EQ(output[(y * stride + x) * 4 + 2], 0) << "x=" << x << ", y=" << y;
      EXPECT_EQ(output[(y * stride + x) * 4 + 3], 255) << "x=" << x << ", y=" << y;
    }
  }
}

TEST(PixelmatchDeathTest, NegativeDimensions) {
  std::array<uint8_t, 8> img1;
  std::array<uint8_t, 8> img2;
  EXPECT_DEBUG_DEATH(pixelmatch(img1, img2, pixelmatch::span<uint8_t>(), -1, 2, 1, Options()),
                     "width > 0");
  EXPECT_DEBUG_DEATH(pixelmatch(img1, img2, pixelmatch::span<uint8_t>(), 1, -2, 1, Options()),
                     "height > 0");
}

TEST(PixelmatchDeathTest, MismatchedImageDataSizes) {
  {
    std::array<uint8_t, 8> img1;
    std::array<uint8_t, 9> img2;
    EXPECT_DEBUG_DEATH(pixelmatch(img1, img2, pixelmatch::span<uint8_t>(), 2, 1, 2, Options()),
                       "Image data size does not match width/height");
  }

  {
    std::array<uint8_t, 9> img1;
    std::array<uint8_t, 8> img2;
    EXPECT_DEBUG_DEATH(pixelmatch(img1, img2, pixelmatch::span<uint8_t>(), 2, 1, 2, Options()),
                       "Image data size does not match width/height");
  }
}

TEST(PixelmatchDeathTest, InvalidOutputSize) {
  std::array<uint8_t, 8> img1;
  std::array<uint8_t, 8> img2;
  std::array<uint8_t, 9> output;
  EXPECT_DEBUG_DEATH(pixelmatch(img1, img2, output, 2, 1, 2, Options()),
                     "img1\\.size\\(\\) == output\\.size\\(\\)");
}

TEST(PixelmatchDeathTest, InvalidStride) {
  std::array<uint8_t, 48> img1;
  std::array<uint8_t, 48> img2;
  EXPECT_DEBUG_DEATH(pixelmatch(img1, img2, pixelmatch::span<uint8_t>(), 4, 4, 3, Options()),
                     "Stride must be greater than width");
}

TEST(Pixelmatch, SingleChannelDifferences) {
  EXPECT_TRUE(compareSinglePixel(Color{0, 0, 0, 255}, Color{0, 0, 0, 255}));

  // With large channel differences.
  EXPECT_FALSE(compareSinglePixel(Color{0, 0, 0, 255}, Color{255, 0, 0, 255}));
  EXPECT_FALSE(compareSinglePixel(Color{0, 0, 0, 255}, Color{0, 255, 0, 255}));
  EXPECT_FALSE(compareSinglePixel(Color{0, 0, 0, 255}, Color{0, 0, 255, 255}));

  // With small channel differences.
  EXPECT_FALSE(compareSinglePixel(Color{0, 0, 0, 255}, Color{1, 0, 0, 255}));
  EXPECT_FALSE(compareSinglePixel(Color{0, 0, 0, 255}, Color{0, 1, 0, 255}));
  EXPECT_FALSE(compareSinglePixel(Color{0, 0, 0, 255}, Color{0, 0, 1, 255}));
}

TEST(Pixelmatch, DifferencesInTransparentPixels) {
  // Color channels have no effect if alpha is zero.
  EXPECT_TRUE(compareSinglePixel(Color{0, 0, 0, 0}, Color{255, 0, 0, 0}));
  EXPECT_TRUE(compareSinglePixel(Color{0, 0, 0, 0}, Color{0, 255, 0, 0}));
  EXPECT_TRUE(compareSinglePixel(Color{0, 0, 0, 0}, Color{0, 0, 255, 0}));
}

}  // namespace pixelmatch
