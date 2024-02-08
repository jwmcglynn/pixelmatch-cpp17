#include "pixelmatch/image_utils.h"

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <cassert>
#include <cstring>  // For memcmp.
#include <fstream>

namespace pixelmatch {

std::optional<Image> readRgbaImageFromPngFile(const char* filename) noexcept {
  int width, height, channels;
  auto data = stbi_load(filename, &width, &height, &channels, 4);
  if (!data) {
    return std::nullopt;
  }

  Image result{width, height, static_cast<size_t>(width),
               std::vector<uint8_t>(data, data + width * height * 4)};
  stbi_image_free(data);
  return result;
}

bool writeRgbaPixelsToPngFile(const char* filename, span<const uint8_t> rgbaPixels, int width,
                              int height, size_t strideInPixels) noexcept {
  struct Context {
    std::ofstream output;
  };

  assert(rgbaPixels.size() == strideInPixels * height * 4);

  Context context;
  context.output = std::ofstream(filename, std::ofstream::out | std::ofstream::binary);
  if (!context.output) {
    return false;
  }

  stbi_write_png_to_func(
      [](void* context, void* data, int len) noexcept {
        Context* contextObj = reinterpret_cast<Context*>(context);
        contextObj->output.write(static_cast<const char*>(data), len);
      },
      &context, width, height, 4, rgbaPixels.data(), strideInPixels * 4);

  return context.output.good();
}

bool imageEquals(span<const uint8_t> img1, span<const uint8_t> img2, int width, int height,
                 size_t strideInPixels) noexcept {
  // Check for identical images, respecting stride.
  for (int y = 0; y < height; ++y) {
    const size_t rowStartIndex = y * strideInPixels;
    if (std::memcmp(&img1[rowStartIndex * 4], &img2[rowStartIndex * 4], width * 4) != 0) {
      return false;
    }
  }

  return true;
}

}  // namespace pixelmatch
