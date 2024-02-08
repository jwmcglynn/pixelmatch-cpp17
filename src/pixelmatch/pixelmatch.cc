
#include "pixelmatch/pixelmatch.h"

#include <algorithm>
#include <cassert>
#include <cstring>  // For memcmp.

namespace pixelmatch {

namespace {

static constexpr size_t kPixelBytes = 4;

inline float rgb2y(uint8_t r, uint8_t g, uint8_t b) noexcept {
  return r * 0.29889531f + g * 0.58662247f + b * 0.11448223f;
}

inline float rgb2i(uint8_t r, uint8_t g, uint8_t b) noexcept {
  return r * 0.59597799f - g * 0.27417610f - b * 0.32180189f;
}

inline float rgb2q(uint8_t r, uint8_t g, uint8_t b) noexcept {
  return r * 0.21147017f - g * 0.52261711f + b * 0.31114694f;
}

/**
 * Blend semi-transparent color with white.
 *
 * @param color The color to blend.
 * @param alpha The alpha value of the color, between 0 and 1.
 * @return The blended color.
 */
inline uint8_t blend(uint8_t c, float a) noexcept {
  return static_cast<uint8_t>(255.0f + (static_cast<float>(c) - 255.0f) * a);
}

/**
 * Calculate color difference according to the paper "Measuring perceived color difference
 * using YIQ NTSC transmission color space in mobile applications" by Y. Kotsarenko and F. Ramos
 *
 * @param img1 The first image, with RGBA-encoded pixels with unpremultiplied alpha.
 * @param img2 The second image with the same size and format as img1.
 * @param pos1 The position in the \ref img1 buffer to start, in bytes. Should point to the start of
 *              an RGBA-encoded pixel.
 * @param pos2 The position in the \ref img2 buffer, same as \ref pos1.
 * @param yOnly Check for brightness difference only.
 * @return the delta, with sign indicating whether the pixel lightens or darkens the pixel lightens
 *          or darkens (positive if img2 lightens). Returns 0 if the pixels are identical.
 */
float colorDelta(span<const uint8_t> img1, span<const uint8_t> img2, size_t pos1, size_t pos2,
                 bool yOnly) noexcept {
  uint8_t r1 = img1[pos1 + 0];
  uint8_t g1 = img1[pos1 + 1];
  uint8_t b1 = img1[pos1 + 2];
  const uint8_t a1 = img1[pos1 + 3];

  uint8_t r2 = img2[pos2 + 0];
  uint8_t g2 = img2[pos2 + 1];
  uint8_t b2 = img2[pos2 + 2];
  const uint8_t a2 = img2[pos2 + 3];

  if (r1 == r2 && g1 == g2 && b1 == b2 && a1 == a2) {
    return 0;
  }

  // If there's alpha, blend with a white background.
  if (a1 < 255) {
    const float alpha = a1 / 255.0f;
    r1 = blend(r1, alpha);
    g1 = blend(g1, alpha);
    b1 = blend(b1, alpha);
  }

  if (a2 < 255) {
    const float alpha = a2 / 255.0f;
    r2 = blend(r2, alpha);
    g2 = blend(g2, alpha);
    b2 = blend(b2, alpha);
  }

  const float y1 = rgb2y(r1, g1, b1);
  const float y2 = rgb2y(r2, g2, b2);
  const float y = y1 - y2;

  if (yOnly) {
    return y;  // Brightness difference only.
  }

  const float i = rgb2i(r1, g1, b1) - rgb2i(r2, g2, b2);
  const float q = rgb2q(r1, g1, b1) - rgb2q(r2, g2, b2);

  const float delta = 0.5053f * y * y + 0.299f * i * i + 0.1957f * q * q;

  // Encode whether the pixel lightens or darkens in the sign.
  return y1 > y2 ? -delta : delta;
}

/// Check if a pixel has 3+ adjacent pixels of the same color.
bool hasManySiblings(span<const uint8_t> img, int x1, int y1, int width, int height,
                     size_t strideInPixels) {
  const int x0 = std::max(x1 - 1, 0);
  const int y0 = std::max(y1 - 1, 0);
  const int x2 = std::min(x1 + 1, width - 1);
  const int y2 = std::min(y1 + 1, height - 1);
  const size_t pos = (y1 * strideInPixels + x1) * kPixelBytes;

  size_t zeroes = x1 == x0 || x1 == x2 || y1 == y0 || y1 == y2 ? 1 : 0;

  // Go through 8 adjacent pixels.
  for (int x = x0; x <= x2; ++x) {
    for (int y = y0; y <= y2; ++y) {
      if (x == x1 && y == y1) {
        continue;
      }

      const size_t pos2 = (y * strideInPixels + x) * kPixelBytes;
      if (img[pos] == img[pos2] && img[pos + 1] == img[pos2 + 1] && img[pos + 2] == img[pos2 + 2] &&
          img[pos + 3] == img[pos2 + 3]) {
        zeroes++;
      }

      if (zeroes > 2) {
        return true;
      }
    }
  }

  return false;
}

/**
 * Check if a pixel is likely a part of anti-aliasing;
 * based on "Anti-aliased Pixel and Intensity Slope Detector" paper by V. Vysniauskas, 2009
 */
bool antialiased(span<const uint8_t> img, int x1, int y1, int width, int height,
                 size_t strideInPixels, span<const uint8_t> img2) noexcept {
  const int x0 = std::max(x1 - 1, 0);
  const int y0 = std::max(y1 - 1, 0);
  const int x2 = std::min(x1 + 1, width - 1);
  const int y2 = std::min(y1 + 1, height - 1);
  const size_t pos = (y1 * strideInPixels + x1) * kPixelBytes;

  size_t zeroes = x1 == x0 || x1 == x2 || y1 == y0 || y1 == y2 ? 1 : 0;
  float minDelta = 0.0f;
  float maxDelta = 0.0f;
  int minX = 0;
  int minY = 0;
  int maxX = 0;
  int maxY = 0;

  // Go through 8 adjacent pixels.
  for (int x = x0; x <= x2; ++x) {
    for (int y = y0; y <= y2; ++y) {
      if (x == x1 && y == y1) {
        continue;
      }

      // Brightness delta between the center pixel and adjacent one.
      const float delta = colorDelta(img, img, pos, (y * strideInPixels + x) * kPixelBytes, true);

      // Count the number of equal, darker and brighter adjacent pixels.
      if (delta == 0) {
        zeroes++;
        // If found more than 2 equal siblings, it's definitely not anti-aliasing.
        if (zeroes > 2) {
          return false;
        }

      } else if (delta < minDelta) {
        // Remember the darkest pixel.
        minDelta = delta;
        minX = x;
        minY = y;

      } else if (delta > maxDelta) {
        // Remember the brightest pixel.
        maxDelta = delta;
        maxX = x;
        maxY = y;
      }
    }
  }

  // If there are no both darker and brighter pixels among siblings, it's not anti-aliasing.
  if (minDelta == 0.0f || maxDelta == 0.0f) {
    return false;
  }

  // If either the darkest or the brightest pixel has 3+ equal siblings in both images
  // (definitely not anti-aliased), this pixel is anti-aliased.
  return (hasManySiblings(img, minX, minY, width, height, strideInPixels) &&
          hasManySiblings(img2, minX, minY, width, height, strideInPixels)) ||
         (hasManySiblings(img, maxX, maxY, width, height, strideInPixels) &&
          hasManySiblings(img2, maxX, maxY, width, height, strideInPixels));
}

inline void drawPixel(span<uint8_t> output, size_t pos, Color color) noexcept {
  output[pos + 0] = color.r;
  output[pos + 1] = color.g;
  output[pos + 2] = color.b;
  output[pos + 3] = color.a;
}

void drawGrayPixel(span<const uint8_t> img, size_t pos, float alpha, span<uint8_t> output) noexcept {
  const uint8_t r = img[pos + 0];
  const uint8_t g = img[pos + 1];
  const uint8_t b = img[pos + 2];
  const uint8_t val = blend(rgb2y(r, g, b), alpha * static_cast<float>(img[pos + 3]) / 255.0f);
  drawPixel(output, pos, Color{val, val, val, 255});
}

}  // namespace

int pixelmatch(span<const uint8_t> img1, span<const uint8_t> img2, span<uint8_t> output, int width,
               int height, size_t strideInPixels, Options options) noexcept {
  // In release builds, return -1 if a precondition fails since the asserts will not trigger.
  if (width <= 0 || height <= 0 || strideInPixels < static_cast<size_t>(width)) {
    assert(width > 0);
    assert(height > 0);
    assert(strideInPixels >= static_cast<size_t>(width) && "Stride must be greater than width");
    return -1;
  }

  if (img1.size() != strideInPixels * height * kPixelBytes || img1.size() != img2.size()) {
    assert(img1.size() == strideInPixels * height * kPixelBytes &&
           "Image data size does not match width/height");
    assert(img2.size() == strideInPixels * height * kPixelBytes &&
           "Image data size does not match width/height");
    return -1;
  }

  if (output.size() != img1.size() && !output.empty()) {
    assert(img1.size() == output.size() || output.empty());
    return -1;
  }

  // Check for identical images, respecting stride.
  bool identical = true;
  for (int y = 0; y < height; ++y) {
    const size_t rowStartIndex = y * strideInPixels;
    if (std::memcmp(&img1[rowStartIndex * kPixelBytes], &img2[rowStartIndex * kPixelBytes],
                    width * 4) != 0) {
      identical = false;
      break;
    }
  }

  // Fast path if identical.
  if (identical) {
    // Update output image, filling with gray pixels.
    if (!output.empty() && !options.diffMask) {
      for (int y = 0; y < height; ++y) {
        const size_t rowStartIndex = y * strideInPixels;
        for (int x = 0; x < width; ++x) {
          const size_t pos = (rowStartIndex + x) * kPixelBytes;
          drawGrayPixel(img1, pos, options.alpha, output);
        }
      }
    }

    return 0;
  }

  // Maximum acceptable square distance between two colors;
  // 35215 is the maximum possible value for the YIQ difference metric
  const float kMaxDelta = 35215.0f * options.threshold * options.threshold;
  int diff = 0;

  // Compare each pixel of one image against the other one.
  for (int y = 0; y < height; ++y) {
    const size_t rowStartIndex = y * strideInPixels;

    for (int x = 0; x < width; ++x) {
      const size_t pos = (rowStartIndex + x) * kPixelBytes;

      // Squared YUV distance between colors at this pixel position, negative if the img2 pixel is
      // darker.
      const float delta = colorDelta(img1, img2, pos, pos, false);

      // The color difference is above the threshold.
      if (std::abs(delta) > kMaxDelta) {
        // Check it's a real rendering difference or just anti-aliasing.
        if (!options.includeAA && (antialiased(img1, x, y, width, height, strideInPixels, img2) ||
                                   antialiased(img2, x, y, width, height, strideInPixels, img1))) {
          // One of the pixels is anti-aliasing; draw as yellow and do not count as difference
          // note that we do not include such pixels in a mask.
          if (!output.empty() && !options.diffMask) {
            drawPixel(output, pos, options.aaColor);
          }
        } else {
          // Found substantial difference not caused by anti-aliasing; draw it as such.
          if (!output.empty()) {
            drawPixel(output, pos,
                      delta < 0.0f && options.diffColorAlt ? options.diffColorAlt.value()
                                                           : options.diffColor);
          }
          diff++;
        }

      } else if (!output.empty()) {
        // Pixels are similar; draw background as grayscale image blended with white.
        if (!options.diffMask) {
          drawGrayPixel(img1, pos, options.alpha, output);
        }
      }
    }
  }

  // Return the number of different pixels.
  return diff;
}

}  // namespace pixelmatch
