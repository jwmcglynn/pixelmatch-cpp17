workspace(name = "pixelmatch-cpp17")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

##
## Third-party dependencies.
##

git_repository(
    name = "com_google_gtest",
    remote = "https://github.com/google/googletest",
    tag = "v1.14.0",
)

load("@com_google_gtest//:googletest_deps.bzl", "googletest_deps")

googletest_deps()

git_repository(
    name = "stb",
    commit = "075e5a470e31e46a7318cc308c79dba205a6b2eb",
    remote = "https://github.com/jwmcglynn/rules_stb",
)

##
## Fuzzer dependencies.
##

http_archive(
    name = "rules_python",
    sha256 = "4f7e2aa1eb9aa722d96498f5ef514f426c1f55161c3c9ae628c857a7128ceb07",
    strip_prefix = "rules_python-1.0.0",
    url = "https://github.com/bazelbuild/rules_python/releases/download/1.0.0/rules_python-1.0.0.tar.gz",
)

load("@rules_python//python:repositories.bzl", "py_repositories")

py_repositories()

http_archive(
    name = "rules_fuzzing",
    sha256 = "e6bc219bfac9e1f83b327dd090f728a9f973ee99b9b5d8e5a184a2732ef08623",
    strip_prefix = "rules_fuzzing-0.5.2",
    urls = ["https://github.com/bazelbuild/rules_fuzzing/archive/v0.5.2.zip"],
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

rules_fuzzing_init()

load("@fuzzing_py_deps//:requirements.bzl", "install_deps")

install_deps()
