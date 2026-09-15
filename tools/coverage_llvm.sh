#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${1:-$repo_dir/../pixelmatch-coverage}"
mkdir -p "$build_dir"
build_dir="$(cd "$build_dir" && pwd)"
profile_dir="$(mktemp -d "$build_dir/profiles.XXXXXX")"

cmake -S "$repo_dir" -B "$build_dir" -G Ninja \
  -DPIXELMATCH_BUILD_TESTS=ON -DPIXELMATCH_BUILD_ALLOCATION_TESTS=ON -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_CXX_COMPILER="${CXX:-clang++}" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS='-fprofile-instr-generate -fcoverage-mapping -ffp-contract=off' \
  -DCMAKE_CXX_FLAGS_RELEASE='-O0 -DNDEBUG'
cmake --build "$build_dir" --parallel
LLVM_PROFILE_FILE="$profile_dir/%p.profraw" TMPDIR="$profile_dir" \
  ctest --test-dir "$build_dir" --output-on-failure
"${LLVM_PROFDATA:-llvm-profdata}" merge -sparse "$profile_dir"/*.profraw \
  -o "$build_dir/coverage.profdata"
coverage_args=("$build_dir/pixelmatch_tests" -object "$build_dir/image_utils_tests"
  -object "$build_dir/pixelmatch_allocation_tests"
  -instr-profile "$build_dir/coverage.profdata" "$repo_dir/src/pixelmatch/")
"${LLVM_COV:-llvm-cov}" report "${coverage_args[@]}"
"${LLVM_COV:-llvm-cov}" export "${coverage_args[@]}" > "$build_dir/coverage.json"
"${LLVM_COV:-llvm-cov}" export -format=lcov "${coverage_args[@]}" > "$build_dir/coverage.dat"
"${LLVM_COV:-llvm-cov}" show -format=html -output-dir="$build_dir/html" "${coverage_args[@]}"
python3 "$repo_dir/tools/check_coverage.py" "$build_dir/coverage.json"
