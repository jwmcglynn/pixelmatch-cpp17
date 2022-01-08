#!/bin/bash -e
cd "$(dirname "$0")/.."

JAVA_HOME=$(dirname $(dirname $(which java)))

bazel coverage -s --nocache_test_results //...

genhtml bazel-out/_coverage/_coverage_report.dat \
  --highlight \
  --legend \
  --output-directory coverage-report
