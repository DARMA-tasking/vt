#!/usr/bin/env bash

set -exo pipefail

source_dir=${1}
build_dir=${2}

export VT=${source_dir}
export VT_BUILD=${build_dir}/vt
pushd "$VT_BUILD"

ctest --output-on-failure | tee cmake-output.log

if test "${CODE_COVERAGE:-0}" -eq 1
then
    export CODECOV_TOKEN="bc653fec-7e2c-412a-8b4f-b6db6d703a02"
    lcov --capture --directory . --output-file coverage.info
    lcov --remove coverage.info '/usr/*' --output-file coverage.info
    lcov --list coverage.info
    pushd "$VT"
    bash <(curl -s https://codecov.io/bash) -f "${VT_BUILD}/coverage.info" || echo "Codecov did not collect coverage reports"
    popd
fi

popd
