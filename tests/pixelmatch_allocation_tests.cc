// Isolated allocation-failure test. Never link these replacements into other test executables.
#include <pixelmatch/pixelmatch.h>

#include <array>
#include <cstdlib>
#include <new>

namespace {
thread_local int allocationsUntilFailure = -1;
thread_local int allocationAttempts = 0;

void* allocate(size_t size) {
  if (allocationsUntilFailure >= 0) {
    ++allocationAttempts;
  }
  if (allocationsUntilFailure == 0) {
    throw std::bad_alloc();
  }
  if (allocationsUntilFailure > 0) {
    --allocationsUntilFailure;
  }
  if (void* result = std::malloc(size == 0 ? 1 : size)) {
    return result;
  }
  throw std::bad_alloc();
}
}  // namespace

void* operator new(size_t size) {
  return allocate(size);
}
void* operator new[](size_t size) {
  return allocate(size);
}
void operator delete(void* ptr) noexcept {
  std::free(ptr);
}
void operator delete[](void* ptr) noexcept {
  std::free(ptr);
}
void operator delete(void* ptr, size_t) noexcept {
  std::free(ptr);
}
void operator delete[](void* ptr, size_t) noexcept {
  std::free(ptr);
}

int main() {
  std::array<uint8_t, 16> first{};
  std::array<uint8_t, 16> second;
  second.fill(255);
  std::array<uint8_t, 16> unchanged;
  unchanged.fill(37);
  pixelmatch::Options options;
  options.includeAA = true;
  options.windowSize = 1;

  // Fail the mask allocation, then the column-sum allocation, without consuming real memory.
  for (int successfulAllocations : {0, 1}) {
    auto output = unchanged;
    allocationAttempts = 0;
    allocationsUntilFailure = successfulAllocations;
    const int result = pixelmatch::pixelmatch(first, second, output, 2, 2, 2, options);
    allocationsUntilFailure = -1;
    if (result != -1 || output != unchanged || allocationAttempts != successfulAllocations + 1) {
      return 1;
    }
  }

  // Recover normally once the fault is disarmed.
  auto output = unchanged;
  if (pixelmatch::pixelmatch(first, second, output, 2, 2, 2, options) != 1 || output == unchanged) {
    return 2;
  }

  // The ordinary path and identical-image fast path must still work without allocating.
  options.windowSize = std::numeric_limits<double>::infinity();
  allocationAttempts = 0;
  allocationsUntilFailure = 0;
  const int total = pixelmatch::pixelmatch(first, second, {}, 2, 2, 2, options);
  options.windowSize = 1;
  const int identical = pixelmatch::pixelmatch(first, first, {}, 2, 2, 2, options);
  allocationsUntilFailure = -1;
  return total == 4 && identical == 0 && allocationAttempts == 0 ? 0 : 3;
}
