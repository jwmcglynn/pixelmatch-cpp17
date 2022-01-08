load("@rules_cc//cc:defs.bzl", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")

cc_test(
    name = "pixelmatch_tests",
    srcs = [
        "pixelmatch_tests.cc",
    ],
    data = glob([
        "testdata/*.png",
    ]),
    deps = [
        "//:image_utils",
        "//:pixelmatch-cpp17",
        "@com_google_gtest//:gtest_main",
    ],
)

cc_test(
    name = "image_utils_tests",
    srcs = [
        "image_utils_tests.cc",
    ],
    data = glob([
        "testdata/*.png",
    ]),
    deps = [
        "//:image_utils",
        "@com_google_gtest//:gtest_main",
    ],
)

cc_fuzz_test(
    name = "pixelmatch_fuzzer",
    srcs = ["pixelmatch_fuzzer.cc"],
    tags = [
        "nocoverage",
    ],
    deps = [
        "//:pixelmatch-cpp17",
    ],
)
