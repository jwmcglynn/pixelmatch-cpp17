#include <gmock/gmock.h>
#include <gtest/gtest-death-test.h>
#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>

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
            << ", diffMask=" << options.diffMask << ", checkerboard=" << options.checkerboard
            << "}";
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

template <size_t kBytes>
void setPixel(std::array<uint8_t, kBytes>& destination, int offsetInPixels, Color color) {
  ASSERT_GE(offsetInPixels, 0);
  ASSERT_LT(static_cast<size_t>(offsetInPixels), kBytes / 4);

  const int offset = offsetInPixels * 4;

  destination[offset + 0] = color.r;
  destination[offset + 1] = color.g;
  destination[offset + 2] = color.b;
  destination[offset + 3] = color.a;
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

  setPixel(img1, 0, pixel1);
  setPixel(img2, 0, pixel2);

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
           defaultTestOptions(), 152);
}

TEST(Pixelmatch, Validate1DiffMask) {
  Options options;
  options.threshold = 0.05f;
  options.includeAA = false;
  options.diffMask = true;

  diffTest("tests/testdata/1a.png", "tests/testdata/1b.png", "tests/testdata/1diffmask.png",
           options, 152);
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
           12821);
}

TEST(Pixelmatch, Validate3Diff) {
  diffTest("tests/testdata/3a.png", "tests/testdata/3b.png", "tests/testdata/3diff.png",
           defaultTestOptions(), 220);
}

TEST(Pixelmatch, Validate4Diff) {
  diffTest("tests/testdata/4a.png", "tests/testdata/4b.png", "tests/testdata/4diff.png",
           defaultTestOptions(), 36563);
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
           2440);
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

  // Transparency-only changes still count.
  EXPECT_FALSE(compareSinglePixel(Color{0, 0, 0, 0}, Color{0, 0, 0, 128}));
}

TEST(Pixelmatch, AlphaDifferenceInAdjacentPixel) {
  constexpr int width = 4;
  constexpr int height = 1;
  constexpr size_t strideInPixels = width;

  constexpr size_t bytes = strideInPixels * height * 4;

  std::array<uint8_t, bytes> img1{};
  std::array<uint8_t, bytes> img2{};
  std::array<uint8_t, bytes> output{};

  Options options;
  options.threshold = 0;

  setPixel(img1, 0, Color{50, 25, 0, 255});
  setPixel(img1, 1, Color{50, 50, 0, 255});
  setPixel(img1, 2, Color{50, 70, 0, 255});

  setPixel(img2, 0, Color{50, 25, 0, 255});
  setPixel(img2, 1, Color{50, 70, 0, 255});
  setPixel(img2, 2, Color{50, 70, 0, 128});

  EXPECT_EQ(pixelmatch(img1, img2, output, width, height, strideInPixels, options), 2);
}

TEST(Pixelmatch, ColorDifferenceInAdjacentPixel) {
  constexpr int width = 4;
  constexpr int height = 1;
  constexpr size_t strideInPixels = width;

  constexpr size_t bytes = strideInPixels * height * 4;

  std::array<uint8_t, bytes> img1{};
  std::array<uint8_t, bytes> img2{};
  std::array<uint8_t, bytes> output{};

  Options options;
  options.threshold = 0;

  const Color adjacentPixelColor{50, 70, 0, 255};
  setPixel(img1, 0, Color{50, 25, 0, 255});
  setPixel(img1, 1, Color{50, 70, 0, 255});
  setPixel(img1, 2, adjacentPixelColor);

  setPixel(img2, 0, Color{50, 25, 0, 255});
  setPixel(img2, 1, Color{50, 50, 0, 255});
  setPixel(img2, 2, adjacentPixelColor);

  Color testedPixelColor = adjacentPixelColor;
  testedPixelColor.r += 10;
  setPixel(img1, 2, testedPixelColor);
  EXPECT_EQ(pixelmatch(img1, img2, output, width, height, strideInPixels, options), 2);

  testedPixelColor = adjacentPixelColor;
  testedPixelColor.g += 10;
  setPixel(img1, 2, testedPixelColor);
  EXPECT_EQ(pixelmatch(img1, img2, output, width, height, strideInPixels, options), 2);

  testedPixelColor = adjacentPixelColor;
  testedPixelColor.b += 10;
  setPixel(img1, 2, testedPixelColor);
  EXPECT_EQ(pixelmatch(img1, img2, output, width, height, strideInPixels, options), 2);
}

TEST(Pixelmatch, ValidateDefaultThreshold) {
  diffTest("tests/testdata/1a.png", "tests/testdata/1b.png",
           "tests/testdata/1diffdefaultthreshold.png", Options(), 121);
}

TEST(Pixelmatch, ValidateTransparentDiff) {
  diffTest("tests/testdata/8a.png", "tests/testdata/5b.png", "tests/testdata/8diff.png",
           defaultTestOptions(), 32896);
}

TEST(Pixelmatch, CheckerboardAndWhiteBackground) {
  std::array<uint8_t, 4> transparentBlack{0, 0, 0, 128};
  std::array<uint8_t, 4> opaqueGray{127, 127, 127, 255};
  Options options;
  EXPECT_EQ(pixelmatch(transparentBlack, opaqueGray, {}, 1, 1, 1, options), 1);
  options.checkerboard = false;
  EXPECT_EQ(pixelmatch(transparentBlack, opaqueGray, {}, 1, 1, 1, options), 0);
  options.threshold = 0;
  EXPECT_EQ(pixelmatch(transparentBlack, opaqueGray, {}, 1, 1, 1, options), 0);
}

TEST(Pixelmatch, FractionalAlphaIsNotRoundedBeforeComparison) {
  std::array<uint8_t, 4> img1{0, 0, 0, 1};
  std::array<uint8_t, 4> img2{1, 1, 1, 1};
  Options options;
  options.threshold = 0;
  options.checkerboard = false;
  EXPECT_EQ(pixelmatch(img1, img2, {}, 1, 1, 1, options), 1);
}

TEST(Pixelmatch, PreviousApiRemainsSourceCompatible) {
  // Keep the original positional aggregate initializer, public field types and function signature.
  Options options{0.1f, true, 0.1f, {255, 255, 0, 19}, {255, 0, 0, 29}, Color{0, 255, 0, 39},
                  false};
  static_assert(std::is_same_v<decltype(options.threshold), float>);
  using Function = int (*)(span<const uint8_t>, span<const uint8_t>, span<uint8_t>, int, int,
                           size_t, Options) noexcept;
  Function compare = &pixelmatch;
  std::array<uint8_t, 4> black{0, 0, 0, 255};
  std::array<uint8_t, 4> white{255, 255, 255, 255};
  std::array<uint8_t, 4> output{};
  EXPECT_TRUE(options.checkerboard);
  EXPECT_EQ(compare(black, white, output, 1, 1, 1, options), 1);
  EXPECT_EQ(output, (std::array<uint8_t, 4>{255, 0, 0, 29}));
  EXPECT_EQ(compare(white, black, output, 1, 1, 1, options), 1);
  EXPECT_EQ(output, (std::array<uint8_t, 4>{0, 255, 0, 39}));
  EXPECT_EQ(pixelmatch(black, white, {}, 1, 1, 1), 1);
}

TEST(PixelmatchDeathTest, OversizedDimensions) {
  std::array<uint8_t, 4> image{};
  EXPECT_DEBUG_DEATH(pixelmatch(image, image, {}, std::numeric_limits<int>::max(), 2,
                                std::numeric_limits<int>::max()),
                     "Image dimensions are too large");
  EXPECT_DEBUG_DEATH(pixelmatch(image, image, {}, 1, 1, std::numeric_limits<size_t>::max()),
                     "Image dimensions are too large");
}

TEST(Pixelmatch, InvalidInputReturnsMinusOneInRelease) {
#ifdef NDEBUG
  std::array<uint8_t, 4> image{};
  std::array<uint8_t, 8> larger{};
  EXPECT_EQ(pixelmatch(image, image, {}, 0, 1, 1), -1);
  EXPECT_EQ(pixelmatch(image, image, {}, 1, 0, 1), -1);
  EXPECT_EQ(pixelmatch(image, image, {}, 2, 1, 1), -1);
  EXPECT_EQ(pixelmatch(image, image, {}, std::numeric_limits<int>::max(), 2,
                       std::numeric_limits<int>::max()),
            -1);
  EXPECT_EQ(pixelmatch(image, image, {}, 1, 1, std::numeric_limits<size_t>::max()), -1);
  EXPECT_EQ(pixelmatch(image, image, {}, 2, 1, 2), -1);
  EXPECT_EQ(pixelmatch(image, larger, {}, 1, 1, 1), -1);
  EXPECT_EQ(pixelmatch(image, image, larger, 1, 1, 1), -1);
#endif
}

TEST(Pixelmatch, TransparentWhiteAntiAliasingRamp) {
  std::array<uint8_t, 12> ramp{255, 255, 255, 0, 255, 255, 255, 128, 255, 255, 255, 255};
  std::array<uint8_t, 12> white{};
  white.fill(255);
  Options options;
  options.threshold = 0;
  EXPECT_EQ(pixelmatch(ramp, white, {}, 3, 1, 3, options), 2);
}

TEST(Pixelmatch, OklabNearBlackAndColorSensitivity) {
  std::array<uint8_t, 4> black{0, 0, 0, 255};
  for (uint8_t gray : {uint8_t(13), uint8_t(23)}) {
    std::array<uint8_t, 4> pixel{gray, gray, gray, 255};
    EXPECT_EQ(pixelmatch(black, pixel, {}, 1, 1, 1), gray == 23 ? 1 : 0);
  }
  Options options;
  options.threshold = 0.05f;
  for (int channel = 0; channel < 3; ++channel) {
    auto pixel = black;
    pixel[channel] = 1;
    EXPECT_EQ(pixelmatch(black, pixel, {}, 1, 1, 1, options), 0);
  }
  std::array<uint8_t, 4> visible{32, 20, 20, 255};
  EXPECT_EQ(pixelmatch(black, visible, {}, 1, 1, 1, options), 1);

  // Upstream issue #127: distinguish visible blue changes from nearly identical cyan colors.
  const std::array<std::array<uint8_t, 8>, 4> pairs{{{41, 56, 157, 255, 41, 56, 0, 255},
                                                     {39, 44, 92, 255, 39, 44, 14, 255},
                                                     {0, 254, 252, 255, 94, 254, 252, 255},
                                                     {0, 254, 252, 255, 47, 254, 252, 255}}};
  for (size_t i = 0; i < pairs.size(); ++i) {
    EXPECT_EQ(pixelmatch({pairs[i].data(), 4}, {pairs[i].data() + 4, 4}, {}, 1, 1, 1),
              i < 2 ? 1 : 0);
  }
}

TEST(Pixelmatch, SlidingWindowsCountLocalDifferencesAndPreserveOutput) {
  std::array<uint8_t, 400> img1;
  img1.fill(255);
  auto img2 = img1;
  for (int y = 2; y < 5; ++y) {
    for (int x = 2; x < 5; ++x) {
      setPixel(img2, y * 10 + x, Color{0, 0, 0, 255});
    }
  }
  Options options;
  options.includeAA = true;
  std::array<uint8_t, 400> expected;
  EXPECT_EQ(pixelmatch(img1, img2, expected, 10, 10, 10, options), 9);
  const std::array<double, 11> sizes{0,
                                     -5,
                                     1,
                                     2,
                                     2.9,
                                     3,
                                     100,
                                     2147483648.0,
                                     std::numeric_limits<double>::quiet_NaN(),
                                     std::numeric_limits<double>::infinity(),
                                     -std::numeric_limits<double>::infinity()};
  for (double size : sizes) {
    SCOPED_TRACE(testing::Message() << "windowSize=" << size);
    options.windowSize = size;
    const int count = !std::isfinite(size) ? 9 : size < 2 ? 1 : size < 3 ? 4 : 9;
    std::array<uint8_t, 400> output;
    EXPECT_EQ(pixelmatch(img1, img2, output, 10, 10, 10, options), count);
    EXPECT_EQ(output, expected);
    EXPECT_EQ(pixelmatch(img1, img1, {}, 10, 10, 10, options), 0);
  }
}

TEST(Pixelmatch, SlidingWindowsOnUpstreamFixture) {
  auto img1 = readRgbaImageFromPngFile("tests/testdata/6a.png");
  auto img2 = readRgbaImageFromPngFile("tests/testdata/6b.png");
  ASSERT_TRUE(img1);
  ASSERT_TRUE(img2);
  Options options = defaultTestOptions();
  for (const auto& pair : {std::pair<int, int>{256, 51}, {100000, 51}, {32, 29}, {8, 6}}) {
    options.windowSize = pair.first;
    EXPECT_EQ(pixelmatch(img1->data, img2->data, {}, img1->width, img1->height,
                         img1->strideInPixels, options),
              pair.second);
  }
}

TEST(Pixelmatch, JavaScriptParity) {
  std::ifstream corpus("tests/testdata/parity.txt");
  ASSERT_TRUE(corpus.is_open());
  int cases = 0;
  int width, height, expectedCount;
  bool alternate;
  Options options;
  while (corpus >> width >> height >> options.threshold >> options.alpha >> options.includeAA >>
         options.diffMask >> options.checkerboard >> alternate >> options.windowSize >>
         expectedCount) {
    SCOPED_TRACE(testing::Message() << "Reference case " << cases++ << ", " << options);
    if (options.windowSize == -999) options.windowSize = std::numeric_limits<double>::infinity();
    options.aaColor = Color{0, 192, 0, 255};
    options.diffColor = Color{255, 0, 255, 255};
    options.diffColorAlt = alternate ? std::optional<Color>(Color{0, 128, 255, 255}) : std::nullopt;
    std::array<std::vector<uint8_t>, 3> images;
    for (auto& image : images) {
      std::string hex;
      ASSERT_TRUE(bool(corpus >> hex));
      ASSERT_EQ(hex.size(), size_t(width) * height * 8);
      for (size_t i = 0; i < hex.size(); i += 2) {
        image.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
      }
    }
    for (int padding : {0, 3}) {
      const size_t stride = width + padding;
      std::vector<uint8_t> img1(stride * height * 4, 11);
      std::vector<uint8_t> img2(stride * height * 4, 22);
      std::vector<uint8_t> expected(stride * height * 4, 37);
      for (int y = 0; y < height; ++y) {
        std::copy_n(images[0].begin() + y * width * 4, width * 4, img1.begin() + y * stride * 4);
        std::copy_n(images[1].begin() + y * width * 4, width * 4, img2.begin() + y * stride * 4);
        std::copy_n(images[2].begin() + y * width * 4, width * 4,
                    expected.begin() + y * stride * 4);
      }
      std::vector<uint8_t> output(stride * height * 4, 37);
      EXPECT_EQ(pixelmatch(img1, img2, output, width, height, stride, options), expectedCount);
      EXPECT_EQ(output, expected);
      EXPECT_EQ(pixelmatch(img1, img2, {}, width, height, stride, options), expectedCount);
    }
  }
  EXPECT_TRUE(corpus.eof());
  EXPECT_EQ(cases, 256);
}

#if __cplusplus == 201703L
TEST(Pixelmatch, SpanPolyfillCompatibility) {
  std::array<uint8_t, 4> image{1, 2, 3, 4};
  span<uint8_t> view(image);
  EXPECT_EQ(view.data(), image.data());
  const span<uint8_t> constantView(view);
  EXPECT_EQ(constantView[2], 3);
  EXPECT_EQ(constantView.size(), 4);
  EXPECT_FALSE(constantView.empty());
  view[1] = 7;
  EXPECT_EQ(image[1], 7);
  span<uint8_t> empty;
  EXPECT_EQ(empty.data(), nullptr);
  EXPECT_TRUE(empty.empty());
  empty = view;
  EXPECT_EQ(empty.data(), image.data());
}
#endif

}  // namespace pixelmatch
