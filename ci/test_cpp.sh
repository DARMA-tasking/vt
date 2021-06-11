#!/usr/bin/env bash

set -exo pipefail

source_dir=${1}
build_dir=${2}

export VT=${source_dir}
export VT_BUILD=${build_dir}/vt
pushd "$VT_BUILD"

# Run unit tests only (use label 'unit_test')
ctest --output-on-failure -L unit_test | tee cmake-output.log

if test "${CODE_COVERAGE:-0}" -eq 1
then
    export CODECOV_TOKEN="$CODECOV_TOKEN"
    lcov --capture --directory . --output-file coverage.info
    lcov --remove coverage.info '/usr/*' --output-file coverage.info
    lcov --list coverage.info
    pushd "$VT"
    bash <(curl -s https://codecov.io/bash) -f "${VT_BUILD}/coverage.info" || echo "Codecov did not collect coverage reports"
    popd
fi

popd
