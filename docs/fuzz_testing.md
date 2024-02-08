# Fuzz Testing

This project uses [libFuzzer](https://llvm.org/docs/LibFuzzer.html) for fuzz testing. The fuzzer is run on the `pixelmatch` function with fuzzer-provided inputs.

To run the fuzzer, use the following command:

```sh
bazel run --config=asan-libfuzzer //tests:pixelmatch_fuzzer_run
```
