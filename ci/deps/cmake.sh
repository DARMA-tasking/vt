#!/usr/bin/env bash

set -exo pipefail

if test $# -lt 1
then
    echo "usage: ./$0 <cmake-version> <arch>"
    exit 1
fi

cmake_version=$1
arch=x86_64

if test $# -gt 1
then
    arch=$2
fi

if test "${arch}" = "arm64v8"
then
    arch=aarch64
fi

if test "${arch}" = "amd64"
then
    arch=x86_64
fi

cmake_tar_name=cmake-${cmake_version}-linux-$arch.tar.gz

echo "${cmake_version}"
echo "${cmake_tar_name}"

wget http://github.com/Kitware/CMake/releases/download/v${cmake_version}/${cmake_tar_name}

tar xzf ${cmake_tar_name} --one-top-level=cmake --strip-components 1
