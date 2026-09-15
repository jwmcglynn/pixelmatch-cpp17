
#include "pixelmatch/pixelmatch.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>  // For memcmp.
#include <limits>
#include <memory>
#include <new>

namespace pixelmatch {

namespace {

static constexpr size_t kPixelBytes = 4;

// Match the upstream interpolation tables, including exact cube roots near black.
struct ColorTables {
  static constexpr size_t kCubeRootSamples = 4096;
  std::array<double, 257> linear{};
  std::array<double, kCubeRootSamples + 2> cubeRoot{};

  ColorTables() noexcept {
    for (size_t i = 0; i < 256; ++i) {
      const double c = i / 255.0;
      linear[i] = c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
    }
    linear[256] = linear[255];
    for (size_t i = 0; i < cubeRoot.size(); ++i) {
      cubeRoot[i] = std::cbrt(double(i) / kCubeRootSamples);
    }
  }
};

const ColorTables& colorTables() noexcept {
  static const ColorTables tables;
  return tables;
}

double linearize(double channel) noexcept {
  const auto& table = colorTables().linear;
  const size_t i = static_cast<size_t>(channel);
  return table[i] + (table[i + 1] - table[i]) * (channel - i);
}

double cubeRoot(double value) noexcept {
  const double t = value * ColorTables::kCubeRootSamples;
  const size_t i = static_cast<size_t>(t);
  if (i < 8) {
    return std::cbrt(value);
  }
  const auto& table = colorTables().cubeRoot;
  return table[i] + (table[i + 1] - table[i]) * (t - i);
}

struct PerceptualColor {
  double l;
  double m;
  double s;
  double lightness;
};

PerceptualColor perceptualColor(double r, double g, double b) noexcept {
  const double lr = linearize(r);
  const double lg = linearize(g);
  const double lb = linearize(b);
  const double l = cubeRoot(0.4122214708 * lr + 0.5363325363 * lg + 0.0514459929 * lb);
  const double m = cubeRoot(0.2119034982 * lr + 0.6806995451 * lg + 0.1073969566 * lb);
  const double s = cubeRoot(0.0883024619 * lr + 0.2817188376 * lg + 0.6299787005 * lb);
  const double lightness = 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s;
  constexpr double k1 = 0.206;
  constexpr double k2 = 0.03;
  constexpr double k3 = (1 + k1) / (1 + k2);
  const double x = k3 * lightness - k1;
  const double toe = 0.5 * (x + std::sqrt(x * x + 4 * k2 * k3 * lightness));
  return {l, m, s, toe};
}

/**
 * Compare colors using toe-corrected OKLab HyAB. The sign identifies dark-on-light changes.
 * Background positions use the tightly packed image offset, independent of row padding.
 */
int colorDelta(span<const uint8_t> img1, span<const uint8_t> img2, size_t pos,
               size_t backgroundOffset, bool checkerboard, double threshold) noexcept {
  double r1 = img1[pos];
  double g1 = img1[pos + 1];
  double b1 = img1[pos + 2];
  const double a1 = img1[pos + 3];
  double r2 = img2[pos];
  double g2 = img2[pos + 1];
  double b2 = img2[pos + 2];
  const double a2 = img2[pos + 3];
  if (r1 == r2 && g1 == g2 && b1 == b2 && a1 == a2) {
    return 0;
  }

  if (a1 < 255 || a2 < 255) {
    double rb = 255;
    double gb = 255;
    double bb = 255;
    if (checkerboard) {
      // Unsigned parity keeps backgrounds valid beyond JavaScript's signed 32-bit offset range.
      rb = 48 + 159 * (backgroundOffset % 2);
      gb = 48 + 159 * (static_cast<uint64_t>(backgroundOffset / 1.618033988749895) % 2);
      bb = 48 + 159 * (static_cast<uint64_t>(backgroundOffset / 2.618033988749895) % 2);
    }
    r1 = (r1 * a1 + rb * (255 - a1)) / 255;
    g1 = (g1 * a1 + gb * (255 - a1)) / 255;
    b1 = (b1 * a1 + bb * (255 - a1)) / 255;
    r2 = (r2 * a2 + rb * (255 - a2)) / 255;
    g2 = (g2 * a2 + gb * (255 - a2)) / 255;
    b2 = (b2 * a2 + bb * (255 - a2)) / 255;
  }

  const auto first = perceptualColor(r1, g1, b1);
  const auto second = perceptualColor(r2, g2, b2);
  const double lightness = first.lightness - second.lightness;
  const double rest = threshold - std::abs(lightness);
  if (rest >= 0) {
    const double dl = first.l - second.l;
    const double dm = first.m - second.m;
    const double ds = first.s - second.s;
    const double da = 1.9779984951 * dl - 2.4285922050 * dm + 0.4505937099 * ds;
    const double db = 0.0259040371 * dl + 0.7827717662 * dm - 0.8086757660 * ds;
    if (da * da + db * db <= rest * rest) {
      return 0;
    }
  }
  return lightness > 0 ? -1 : 1;
}

// Anti-aliasing needs a monotonic brightness ramp, composited over fixed white.
double brightnessDelta(span<const uint8_t> img, size_t pos1, size_t pos2) noexcept {
  const double r1 = img[pos1];
  const double g1 = img[pos1 + 1];
  const double b1 = img[pos1 + 2];
  const double a1 = img[pos1 + 3];
  const double r2 = img[pos2];
  const double g2 = img[pos2 + 1];
  const double b2 = img[pos2 + 2];
  const double a2 = img[pos2 + 3];
  double dr = r1 - r2;
  double dg = g1 - g2;
  double db = b1 - b2;
  const double da = a1 - a2;
  if (a1 < 255 || a2 < 255) {
    dr = (r1 * a1 - r2 * a2 - 255 * da) / 255;
    dg = (g1 * a1 - g2 * a2 - 255 * da) / 255;
    db = (b1 * a1 - b2 * a2 - 255 * da) / 255;
    const double d = dr * 0.29889531 + dg * 0.58662247 + db * 0.11448223;
    return d == 0 && da != 0 ? da / 2 : d;
  }
  return dr * 0.29889531 + dg * 0.58662247 + db * 0.11448223;
}

// Count each square window in O(width * height), using O(width) column sums.
int maxWindowDiff(span<const uint8_t> mask, span<int> columns, int width, int height,
                  int size) noexcept {
  int maximum = 0;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      columns[x] += mask[size_t(y) * width + x];
      if (y >= size) {
        columns[x] -= mask[size_t(y - size) * width + x];
      }
    }
    if (y < size - 1) {
      continue;
    }
    int count = 0;
    for (int x = 0; x < width; ++x) {
      count += columns[x];
      if (x >= size) {
        count -= columns[x - size];
      }
      maximum = std::max(maximum, count);
    }
  }
  return maximum;
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
  double minDelta = 0;
  double maxDelta = 0;
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
      const double delta = brightnessDelta(img, pos, (y * strideInPixels + x) * kPixelBytes);

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

void drawGrayPixel(span<const uint8_t> img, size_t pos, float alpha,
                   span<uint8_t> output) noexcept {
  const uint8_t r = img[pos + 0];
  const uint8_t g = img[pos + 1];
  const uint8_t b = img[pos + 2];
  const double gray =
      255 + (r * 0.29889531 + g * 0.58662247 + b * 0.11448223 - 255) * alpha * img[pos + 3] / 255;
  const uint8_t val = static_cast<uint8_t>(gray);
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

  if (width > std::numeric_limits<int>::max() / height ||
      strideInPixels > std::numeric_limits<size_t>::max() / kPixelBytes / height) {
    assert(false && "Image dimensions are too large");
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
                    size_t(width) * kPixelBytes) != 0) {
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

  int diff = 0;
  const bool windowed = std::isfinite(options.windowSize);
  const size_t pixels = size_t(width) * height;
  std::unique_ptr<uint8_t[]> mask;
  std::unique_ptr<int[]> columns;
  if (windowed) {
    // Allocate both buffers before touching output, preserving noexcept on allocation failure.
    mask.reset(new (std::nothrow) uint8_t[pixels]{});
    if (!mask) {
      return -1;
    }
    columns.reset(new (std::nothrow) int[width]{});
    if (!columns) {
      return -1;
    }
  }

  // Compare each pixel of one image against the other one.
  for (int y = 0; y < height; ++y) {
    const size_t rowStartIndex = y * strideInPixels;

    for (int x = 0; x < width; ++x) {
      const size_t pos = (rowStartIndex + x) * kPixelBytes;

      const int delta = colorDelta(img1, img2, pos, (size_t(y) * width + x) * kPixelBytes,
                                   options.checkerboard, options.threshold);

      if (delta != 0) {
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
          if (windowed) {
            mask[size_t(y) * width + x] = 1;
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

  if (windowed) {
    const int size = static_cast<int>(
        std::min(std::max(std::floor(options.windowSize), 1.0), double(std::min(width, height))));
    return maxWindowDiff({mask.get(), pixels}, {columns.get(), size_t(width)}, width, height, size);
  }
  return diff;
}

}  // namespace pixelmatch
