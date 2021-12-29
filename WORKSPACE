load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

##
## Third-party dependencies.
##

git_repository(
    name = "com_google_gtest",
    branch = "main",
    remote = "https://github.com/google/googletest",
)

# Use absl at head.
git_repository(
    name = "com_google_absl",
    branch = "master",
    remote = "https://github.com/abseil/abseil-cpp",
)

git_repository(
    name = "stb",
    branch = "master",
    init_submodules = True,
    remote = "https://github.com/nitronoid/rules_stb",
)
