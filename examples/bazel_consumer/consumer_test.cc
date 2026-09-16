#include <pixelmatch/image_utils.h>
#include <pixelmatch/pixelmatch.h>

#include <array>
#include <type_traits>

int main() {
  std::array<uint8_t, 16> black{0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255};
  auto changed = black;
  changed[0] = 255;
  std::array<uint8_t, 16> output{};

  // Preserve the original aggregate initializer and noexcept API in a downstream build.
  pixelmatch::Options options{0.1f,         true, 0.1f, {255, 255, 0, 255}, {255, 0, 0, 255},
                              std::nullopt, false};
  static_assert(noexcept(pixelmatch::pixelmatch(black, changed, output, 2, 2, 2, options)));
  static_assert(std::is_same_v<decltype(options.threshold), float>);
  if (pixelmatch::pixelmatch(black, changed, output, 2, 2, 2, options) != 1) {
    return 1;
  }
  options.windowSize = 1;
  if (pixelmatch::pixelmatch(black, changed, {}, 2, 2, 2, options) != 1) {
    return 2;
  }
  options.checkerboard = false;
  if (pixelmatch::pixelmatch(black, black, {}, 2, 2, 2, options) != 0) {
    return 3;
  }
  if (!pixelmatch::imageEquals(black, black, 2, 2, 2) ||
      pixelmatch::imageEquals(black, changed, 2, 2, 2)) {
    return 4;
  }
  return 0;
}
