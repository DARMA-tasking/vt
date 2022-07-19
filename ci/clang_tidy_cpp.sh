#!/usr/bin/env bash

set -ex

build_dir=${1}
pushd "${build_dir}/vt"

# That's just an example of how to set checks
run-clang-tidy -checks='-*, bugprone-easily-swappable-parameters'
