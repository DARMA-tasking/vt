#!/usr/bin/env bash

set -exo pipefail

if test $# -lt 1
then
    echo "usage: ./$0 <fmt-version> <make_flags>"
    exit 1
fi

fmt_version=$1

base_dir=$(pwd)

git clone https://github.com/fmtlib/fmt.git
cd fmt
git checkout ${fmt_version}
mkdir build && cd build
cmake .. -D FMT_TEST=OFF
make ${make_flags}
make install
cd ${base_dir}
rm -rf fmt
