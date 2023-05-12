# pixelmatch-cpp17

[![Build Status](https://github.com/jwmcglynn/pixelmatch-cpp17/actions/workflows/main.yml/badge.svg?branch=main)](https://github.com/jwmcglynn/pixelmatch-cpp17/actions/workflows/main.yml) [![License: ISC](https://img.shields.io/badge/License-ISC-blue.svg)](https://opensource.org/licenses/ISC) [![codecov](https://codecov.io/gh/jwmcglynn/pixelmatch-cpp17/branch/main/graph/badge.svg?token=0XMUH3F0RD)](https://codecov.io/gh/jwmcglynn/pixelmatch-cpp17)

A C++17 port of the JavaScript pixelmatch library, providing a small pixel-level image comparison library.

Features accurate **anti-aliased pixels detection** and **perceptual color difference metrics**.

Based on [mapbox/pixelmatch](https://github.com/mapbox/pixelmatch).  pixelmatch-cpp17 is around **300 lines of code**, and has **no dependencies**, operating on RGBA-encoded buffers.


```cpp
pixelmatch::Options options;
options.threshold = 0.1f;

const std::vector<uint8_t> img1 = ...;
const std::vector<uint8_t> img2 = ...;
std::vector<uint8_t> diffImage(img1.size());

const int numDiffPixels = pixelmatch::pixelmatch(img1, img2, diffImage, width, height, stride, options);
```

Compared to [mapbox/pixelmatch-cpp](https://github.com/mapbox/pixelmatch-cpp), pixelmatch-cpp17 ports the latest features from the JavaScript library, and is built with production-grade practices, including thorough test coverage and fuzz-testing.  Build files are included for Bazel, but contributions for other build systems are welcome.

Implements ideas from the following papers:

- [Measuring perceived color difference using YIQ NTSC transmission color space in mobile applications](http://www.progmat.uaem.mx:8080/artVol2Num2/Articulo3Vol2Num2.pdf) (2010, Yuriy Kotsarenko, Fernando Ramos)
- [Anti-aliased pixel and intensity slope detector](https://www.researchgate.net/publication/234126755_Anti-aliased_Pixel_and_Intensity_Slope_Detector) (2009, Vytautas Vyšniauskas)

## Example output

| expected | actual | diff |
| --- | --- | --- |
| ![](tests/testdata/4a.png) | ![](tests/testdata/4b.png) | ![1diff](tests/testdata/4diff.png) |
| ![](tests/testdata/3a.png) | ![](tests/testdata/3b.png) | ![1diff](tests/testdata/3diff.png) |
| ![](tests/testdata/6a.png) | ![](tests/testdata/6b.png) | ![1diff](tests/testdata/6diff.png) |

## API

### pixelmatch(img1, img2, output, width, height, strideInPixels[, options])

- `img1`, `img2` — Image data of the images to compare, as a RGBA-encoded byte array. **Note:** image dimensions must be equal.
- `output` — Image data to write the diff to, or `std::nullopt` if you don't need a diff image.
- `width`, `height` — Width and height of the images. Note that _all three images_ need to have the same dimensions.
- `strideInPixels` — Stride of the images. Note that _all three images_ need to have the same stride.
- `options` is a struct with the following fields:
  - `threshold` — Matching threshold, ranges from `0.0f` to `1.0f`. Smaller values make the comparison more sensitive. `0.1` by default.
  - `includeAA` — If `true`, disables detecting and ignoring anti-aliased pixels. `false` by default.
  - `alpha` — Blending factor of unchanged pixels in the diff output. Ranges from `0` for pure white to `1` for original brightness. `0.1` by default.
  - `aaColor` — The color of anti-aliased pixels in the diff output as an RGBA color. `(255, 255, 0, 255)` by default.
  - `diffColor` — The color of differing pixels in the diff output as an RGBA color `(255, 0, 0, 255)` by default.
  - `diffColorAlt` — An alternative color to use for dark on light differences to differentiate between "added" and "removed" parts. If not provided, all differing pixels use the color specified by `diffColor`. `std::nullopt` by default.
  - `diffMask` — Draw the diff over a transparent background (a mask), rather than over the original image. Will not draw anti-aliased pixels (if detected).

Compares two images, writes the output diff and returns the number of mismatched pixels.

## Usage

### Bazel

Add the following to your `WORKSPACE` file:
```py
git_repository(
    name = "pixelmatch-cpp17",
    branch = "main",
    remote = "https://github.com/jwmcglynn/pixelmatch-cpp17",
)
```

Then add a dependency on `@pixelmatch-cpp17`:
```py
cc_test(
    name = "my_test",
    # ...
    data = glob([
      "testdata/*.png",
    ]),
    deps = [
        "@pixelmatch-cpp17",
        # ...
    ],
)
```

In your test file, include pixelmatch with:
```cpp
#include <pixelmatch/pixelmatch.h>
```
