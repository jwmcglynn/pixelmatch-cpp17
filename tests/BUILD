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
        "//:pixelmatch-cpp",
        "@com_google_gtest//:gtest_main",
    ],
)

cc_fuzz_test(
    name = "pixelmatch_fuzzer",
    srcs = ["pixelmatch_fuzzer.cc"],
    deps = [
        "//:pixelmatch-cpp",
    ],
)
