#!/usr/bin/env bash

set -exo pipefail

source_dir=${1}
build_dir=${2}

export VT=${source_dir}
export VT_BUILD=${build_dir}/vt
pushd "$VT_BUILD"

ctest --output-on-failure | tee cmake-output.log

popd
