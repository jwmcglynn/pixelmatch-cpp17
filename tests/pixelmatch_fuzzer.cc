#include <fuzzer/FuzzedDataProvider.h>
#include <pixelmatch/pixelmatch.h>

namespace pixelmatch {
namespace {

Color createColor(FuzzedDataProvider& provider) {
  return Color{provider.ConsumeIntegral<uint8_t>(), provider.ConsumeIntegral<uint8_t>(),
               provider.ConsumeIntegral<uint8_t>(), provider.ConsumeIntegral<uint8_t>()};
}

Options createOptions(FuzzedDataProvider& provider) {
  Options options;
  options.threshold = provider.ConsumeFloatingPoint<float>();
  options.includeAA = provider.ConsumeBool();
  options.alpha = provider.ConsumeFloatingPoint<float>();
  options.aaColor = createColor(provider);
  options.diffColor = createColor(provider);
  if (provider.ConsumeBool()) {
    options.diffColorAlt = createColor(provider);
  }
  options.diffMask = provider.ConsumeBool();
  return options;
}

std::vector<uint8_t> consumePixels(FuzzedDataProvider& provider, size_t size) {
  std::vector<uint8_t> pixels = provider.ConsumeBytes<uint8_t>(size);
  pixels.resize(size);  // Expand to always return the same number of pixels, even if there is not
                        // enough data remaining.
  return pixels;
}

}  // namespace

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t dataSize) {
  FuzzedDataProvider provider(data, dataSize);

  const Options options = createOptions(provider);
#ifdef NDEBUG
  const int width = provider.ConsumeIntegralInRange<int>(-100, 100);
  const int height = provider.ConsumeIntegralInRange<int>(-100, 100);
  const size_t strideInPixels =
      provider.ConsumeIntegralInRange<size_t>(static_cast<size_t>(std::max(width, 0)), 100);
#else
  // Use a stricter set of ranges on debug builds to avoid hitting asserts.
  const int width = provider.ConsumeIntegralInRange<int>(1, 100);
  const int height = provider.ConsumeIntegralInRange<int>(1, 100);
  const size_t strideInPixels =
      provider.ConsumeIntegralInRange<size_t>(static_cast<size_t>(width), 100);
#endif

  const size_t size =
      strideInPixels * std::abs(height) *
      4;  // Take the abs of the height for cases where we generate a negative number on NDEBUG.
  const std::vector<uint8_t> img1 = consumePixels(provider, size);
  const std::vector<uint8_t> img2 = consumePixels(provider, size);

  // Run once without the output.
  std::ignore = pixelmatch(img1, img2, span<uint8_t>(), width, height, strideInPixels, options);

  // Then again with an output buffer.
  std::vector<uint8_t> output(size);
  std::ignore = pixelmatch(img1, img2, output, width, height, strideInPixels, options);

  return 0;
}

}  // namespace pixelmatch
