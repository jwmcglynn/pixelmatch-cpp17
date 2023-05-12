load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

##
## Third-party dependencies.
##

git_repository(
    name = "com_google_gtest",
    remote = "https://github.com/google/googletest",
    tag = "v1.13.0",
)

# Absl at head is currently broken with v1.13.0 of gtest.
# Abseil LTS branch, Jan 2023, Patch 3, https://github.com/abseil/abseil-cpp/releases/tag/20230125.3
git_repository(
    name = "com_google_absl",
    remote = "https://github.com/abseil/abseil-cpp",
    tag = "20230125.3",
)

http_archive(
    name = "com_googlesource_code_re2",
    sha256 = "3664a5d4915f3cf5a9172beb1011dca13d21edc26ce4831a5103140aeae903d2",
    strip_prefix = "re2-c9cba76063cf4235c1a15dd14a24a4ef8d623761",
    urls = ["https://github.com/google/re2/archive/c9cba76063cf4235c1a15dd14a24a4ef8d623761.zip"],
)

git_repository(
    name = "stb",
    branch = "master",
    init_submodules = True,
    remote = "https://github.com/nitronoid/rules_stb",
)

##
## Fuzzer dependencies.
##

http_archive(
    name = "rules_fuzzing",
    sha256 = "23bb074064c6f488d12044934ab1b0631e8e6898d5cf2f6bde087adb01111573",
    strip_prefix = "rules_fuzzing-0.3.1",
    urls = ["https://github.com/bazelbuild/rules_fuzzing/archive/v0.3.1.zip"],
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

rules_fuzzing_init()
