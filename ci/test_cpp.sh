#!/usr/bin/env bash

set -exo pipefail

source_dir=${1}
build_dir=${2}

export VT=${source_dir}
export VT_BUILD=${build_dir}/vt
pushd "$VT_BUILD"

# Don't run performance tests here (use label 'unit_test' or 'example')
ctest --output-on-failure -L 'unit_test|example' | tee cmake-output.log

if test "${VT_CODE_COVERAGE:-0}" -eq 1
then
    export CODECOV_TOKEN="$CODECOV_TOKEN"
    lcov --capture --directory . --output-file coverage.info
    lcov --remove coverage.info '/usr/*' --output-file coverage.info
    lcov --list coverage.info
    pushd "$VT"
    bash <(curl -s https://codecov.io/bash) -f "${VT_BUILD}/coverage.info" || echo "Codecov did not collect coverage reports"
    popd
fi

if test "${VT_CI_TEST_LB_SCHEMA:-0}" -eq 1
then
    echo "Validating schema of json files..."
    "${VT}/scripts/check_lb_data_files.sh" "${VT_BUILD}" "${VT}"
fi

popd
