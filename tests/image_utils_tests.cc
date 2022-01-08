#include <gmock/gmock.h>
#include <gtest/gtest-death-test.h>
#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <fstream>

#include "pixelmatch/image_utils.h"

namespace pixelmatch {

struct AutodeleteFile {
  AutodeleteFile(std::filesystem::path filename) : filename(std::move(filename)) {}
  ~AutodeleteFile() { std::filesystem::remove(filename); }

  std::filesystem::path filename;
};

TEST(ImageUtils, SaveLoad) {
  constexpr int width = 3;
  constexpr int height = 4;
  constexpr size_t stride = 4;

  constexpr size_t bytes = stride * height * 4;

  std::array<uint8_t, bytes> img{};
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      img[(y * stride + x) * 4 + 0] = 128;
      img[(y * stride + x) * 4 + 1] = 128;
      img[(y * stride + x) * 4 + 2] = 128;
      img[(y * stride + x) * 4 + 3] = 255;
    }
  }

  std::filesystem::path savedFilename = std::filesystem::temp_directory_path() / "with-stride.png";
  auto autodelete = AutodeleteFile(savedFilename);

  EXPECT_TRUE(writeRgbaPixelsToPngFile(savedFilename.c_str(), img, width, height, stride));

  // Reload the file and check for equality.
  auto maybeReadImg = readRgbaImageFromPngFile(savedFilename.c_str());
  ASSERT_TRUE(maybeReadImg.has_value());

  const auto& readImg = std::move(maybeReadImg.value());
  EXPECT_EQ(readImg.width, width);
  EXPECT_EQ(readImg.height, height);
  EXPECT_EQ(readImg.strideInPixels, width) << "Stride should equal width when reloading";

  for (int y = 0; y < readImg.height; ++y) {
    for (int x = 0; x < readImg.width; ++x) {
      EXPECT_EQ(readImg.data[(y * readImg.strideInPixels + x) * 4 + 0], 128)
          << "x=" << x << ", y=" << y;
      EXPECT_EQ(readImg.data[(y * readImg.strideInPixels + x) * 4 + 1], 128)
          << "x=" << x << ", y=" << y;
      EXPECT_EQ(readImg.data[(y * readImg.strideInPixels + x) * 4 + 2], 128)
          << "x=" << x << ", y=" << y;
      EXPECT_EQ(readImg.data[(y * readImg.strideInPixels + x) * 4 + 3], 255)
          << "x=" << x << ", y=" << y;
    }
  }
}

TEST(ImageUtils, ReadInvalidPng) {
  // Create an invalid PNG file.
  std::filesystem::path savedFilename = std::filesystem::temp_directory_path() / "invalid.png";
  auto autodelete = AutodeleteFile(savedFilename);

  {
    std::ofstream file(savedFilename.c_str(), std::ios::binary);
    file << "invalid";
  }

  auto maybeReadImg = readRgbaImageFromPngFile(savedFilename.c_str());
  EXPECT_FALSE(maybeReadImg.has_value());
}

TEST(ImageUtils, WriteInvalidFilename) {
  std::filesystem::path directoryName = std::filesystem::temp_directory_path();

  std::array<uint8_t, 4> img;
  EXPECT_FALSE(writeRgbaPixelsToPngFile(directoryName.c_str(), img, 1, 1, 1));
}

TEST(ImageUtils, ImageEquals) {
  std::filesystem::path directoryName = std::filesystem::temp_directory_path();

  std::array<uint8_t, 16> img1{};
  std::array<uint8_t, 16> img2{};

  EXPECT_TRUE(imageEquals(img1, img2, 2, 2, 2));

  img1[8] = 1;
  EXPECT_FALSE(imageEquals(img1, img2, 2, 2, 2));
}

}  // namespace pixelmatch
