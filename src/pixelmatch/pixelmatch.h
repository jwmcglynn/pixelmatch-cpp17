#pragma once

#include <cstdint>
#include <optional>

#if __cplusplus > 201703L
#include <span>
#endif

namespace pixelmatch {

#if __cplusplus > 201703L
// For C++20, use std::span directly.
template <typename T>
using span = std::span<T>;
#else
// For C++17, use a minimal polyfill for std::span sufficient for pixelmatch.
template <typename T>
class span {
public:
  constexpr span() noexcept = default;
  ~span() noexcept = default;

  constexpr span(T* data, size_t size) noexcept : data_(data), size_(size) {}

  template <typename Container,
            std::enable_if_t<
                std::is_pointer<decltype(std::declval<Container&>().data())>::value &&
                    std::is_convertible<
                        std::remove_pointer_t<decltype(std::declval<Container&>().data())> (*)[],
                        T (*)[]>::value,
                int> = 0>
  constexpr span(Container& container) noexcept : span(container.data(), container.size()) {}

  span(const span&) noexcept = default;
  span& operator=(const span&) noexcept = default;

  const T* data() const noexcept { return data_; }
  size_t size() const noexcept { return size_; }
  bool empty() const { return size_ == 0; }

  T operator[](size_t index) const noexcept { return data_[index]; }
  T& operator[](size_t index) noexcept { return data_[index]; }

private:
  T* data_ = nullptr;
  size_t size_ = 0;
};
#endif

/**
 * RGBA-ordered 32-bit color.
 */
struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

/**
 * Pixelmatch options.
 *
 * Defaults to detect anti-aliasing and a small but non-zero threshold of 0.1.
 */
struct Options {
  float threshold = 0.1f;  //!< Matching threshold (0 to 1); smaller is more sensitive
  bool includeAA =
      false;  //!< Includes anti-aliased pixels in the diff, which disables anti-aliasing detection
  float alpha = 0.1f;                  //!< Opacity of original image in diff output
  Color aaColor = {255, 255, 0, 255};  //!< Color of anti-aliased pixels in diff output
  Color diffColor = {255, 0, 0, 255};  //!< Color of different pixels in diff output
  std::optional<Color> diffColorAlt =
      std::nullopt;  //!< Whether to detect dark on light differences between img1 and img2 and set
                     //!< an alternative color to differentiate between the two
  bool diffMask = false;  //!< Draw the diff over a transparent background (a mask)
};

/**
 * Compares two images, optionally detecting anti-aliased pixels and using perceptual color
 * difference metrics.
 *
 * @param img1 First image, as a raw RGBA-ordered pixel buffer. Must be strideInElements * height *
 *              4 bytes long. Assumes that alpha is unpremultiplied.
 * @param img2 Second image, must be the same size as img1.
 * @param output (Optional) Output image buffer, of the same size as img1, or an empty span.
 * @param width in pixels, must be > 0.
 * @param height in pixels, must be > 0.
 * @param strideInElements Stride of the image, in pixels, must be >= width.
 * @param options Configuration options for the pixel comparison algorithm.
 * @return 0 if the images are identical or the number of different pixels if not. If a precondition
 *         fails, returns -1.
 */
int pixelmatch(span<const uint8_t> img1, span<const uint8_t> img2, span<uint8_t> output, int width,
               int height, size_t strideInPixels, Options options = Options()) noexcept;

}  // namespace pixelmatch
