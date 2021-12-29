load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "pixelmatch_tests",
    srcs = [
        "pixelmatch_tests.cc",
    ],
    data = glob([
        "testdata/*.png",
    ]),
    deps = [
        "//:pixelmatch-cpp",
        "@com_google_gtest//:gtest_main",
        "@stb//:image",
        "@stb//:image_write",
    ],
)
