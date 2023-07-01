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
    sha256 = "1d10e8f8746bb5cbd5ac8908b8e48da68598fb8fe2b7650bd7aff583d2ee06cd",
    strip_prefix = "re2-2d39b703d02645076fead8fa409a1711f0e84381",
    urls = ["https://github.com/google/re2/archive/2d39b703d02645076fead8fa409a1711f0e84381.zip"],
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
    sha256 = "d9002dd3cd6437017f08593124fdd1b13b3473c7b929ceb0e60d317cb9346118",
    strip_prefix = "rules_fuzzing-0.3.2",
    urls = ["https://github.com/bazelbuild/rules_fuzzing/archive/v0.3.2.zip"],
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

rules_fuzzing_init()
