# Code Coverage

## Enforced LLVM coverage

Run from the repository root with Clang, matching LLVM tools, CMake, Ninja, and Python 3:

```sh
tools/coverage_llvm.sh ../pixelmatch-coverage
```

Set `CXX`, `LLVM_COV`, and `LLVM_PROFDATA` if the tools have versioned names.
The script runs both complete test executables in C++17 with `NDEBUG` to cover the
release precondition returns, and checks **100% lines, regions, functions, and branches**
across every `src/pixelmatch/*.cc` file and the C++17 span polyfill in `pixelmatch.h`.
Declarations in `image_utils.h` have no executable lines. Tests, dependencies, and toolchain
headers are outside the library coverage denominator. Missing files fail the check.
Debug builds separately test assertions; C++20 builds test `std::span` compatibility.

The build directory contains `coverage.json`, `coverage.dat` (LCOV), and `html/index.html`.
Each run uses fresh profile files so stale execution counts cannot hide a regression.
The Coverage workflow enforces this check before uploading to Codecov.

## Bazel coverage

The existing Bazel/LCOV report is also available:

```sh
tools/coverage.sh
```

It writes HTML to `coverage-report/index.html`.
