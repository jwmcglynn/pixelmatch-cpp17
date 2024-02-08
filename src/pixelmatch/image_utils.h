#pragma once

#include <pixelmatch/pixelmatch.h>

#include <vector>

namespace pixelmatch {

/**
 * Container for Image data returned by \ref readRgbaImageFromPngFile.
 */
struct Image {
  int width;                  //!< Image width in pixels.
  int height;                 //!< Image height in pixels.
  size_t strideInPixels;      //!< Image stride, in pixels. Should be >= width.
  std::vector<uint8_t> data;  //!< Image data as RGBA-encoded pixels, unpremultiplied.
};

/**
 * Reads an image from a PNG file, in a format that can be used by pixelmatch.
 *
 * @param filename Filename to load.
 * @return std::optional<Image> containing the image, or std::nullopt if the file could not be read.
 */
std::optional<Image> readRgbaImageFromPngFile(const char* filename) noexcept;

/**
 * Save an image as a PNG file.
 *
 * @param filename Destination filename.
 * @param rgbaPixels Pixel data, as RGBA-encoded pixels. Alpha should be unpremultiplied.
 * @param width Width of the image.
 * @param height Height of the image.
 * @param strideInPixels Stride of the image pixel data, should be greater than \ref width.
 * @return true If the image was successfully saved.
 */
bool writeRgbaPixelsToPngFile(const char* filename, span<const uint8_t> rgbaPixels, int width,
                              int height, size_t strideInPixels) noexcept;

/**
 * Returns true if two images are bit-identical.
 *
 * @param img1 First image, as 4-byte pixels. The specific order is unimportant, but the image must
 *             be strideInPixels * height * 4 bytes.
 * @param img2 Second image, must be the same size as \ref img1.
 * @param width Image width, in pixels.
 * @param height Image height, in pixels.
 * @param strideInPixels Stride, must be >= width.
 * @return true If the image is bit-identical.
 */
bool imageEquals(span<const uint8_t> img1, span<const uint8_t> img2, int width, int height,
                 size_t strideInPixels) noexcept;

}  // namespace pixelmatch
