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
    sha256 = "aeb2855ad3631b316086b4c32262409dad30679930b2c99e19618023cbe12bca",
    strip_prefix = "re2-a807e8a3aac2cc33c77b7071efea54fcabe38e0c",
    urls = ["https://github.com/google/re2/archive/a807e8a3aac2cc33c77b7071efea54fcabe38e0c.zip"],
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
