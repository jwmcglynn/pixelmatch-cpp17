load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

workspace(name = "pixelmatch-cpp17")

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
    sha256 = "4912ced70dc1a2a8e4b86cec233b192ca053e82bc72d877b98e126156e8f228d",
    strip_prefix = "rules_python-0.32.2",
    url = "https://github.com/bazelbuild/rules_python/releases/download/0.32.2/rules_python-0.32.2.tar.gz",
)

load("@rules_python//python:repositories.bzl", "py_repositories")

py_repositories()

http_archive(
    name = "rules_fuzzing",
    sha256 = "77206c54b71f4dd5335123a6ff2a8ea688eca5378d34b4838114dff71652cf26",
    strip_prefix = "rules_fuzzing-0.5.1",
    urls = ["https://github.com/bazelbuild/rules_fuzzing/archive/v0.5.1.zip"],
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

rules_fuzzing_init()

load("@fuzzing_py_deps//:requirements.bzl", "install_deps")

install_deps()
