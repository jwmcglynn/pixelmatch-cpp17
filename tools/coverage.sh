#!/bin/bash -e
cd "$(dirname "$0")/.."

JAVA_HOME=$(dirname $(dirname $(which java)))

bazel coverage //...

genhtml bazel-out/_coverage/_coverage_report.dat \
  --highlight \
  --legend \
  --branch-coverage \
  --output-directory coverage-report
