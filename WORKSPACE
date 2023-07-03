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
    sha256 = "cea788ff034b09eae288098a645af2fc9b9715c78d1d656a4a1d2ab32a9273bc",
    strip_prefix = "re2-1c1ffbe3c6b2bd26f65b1c1b7bc71d16ba162cf1",
    urls = ["https://github.com/google/re2/archive/1c1ffbe3c6b2bd26f65b1c1b7bc71d16ba162cf1.zip"],
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
