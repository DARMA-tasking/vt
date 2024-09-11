#!/usr/bin/env bash

set -exo pipefail

if test $# -lt 3
then
    echo "usage: ./$0 <vtk-version> <build-root> <make_args>"
    exit 1
fi

base_dir=$(pwd)

vtk_version="$1"
vtk_tar_name="v${vtk_version}.tar.gz"
vtk_name="VTK-${vtk_version}"
build_root="${2-}"

make_args=$3

wget "https://github.com/Kitware/VTK/archive/refs/tags/$vtk_tar_name"
tar xzf ${vtk_tar_name}
rm ${vtk_tar_name}

mkdir -p ${build_root}
cd ${build_root}

mkdir -p build
mkdir -p install

cd build
cmake \
    -DCMAKE_INSTALL_PREFIX:FILEPATH=${build_root}/install \
    ${base_dir}/${vtk_name}
cmake --build . --target install -j ${make_args}

cd ${base_dir}
rm -rf ${vtk_name}
rm -rf ${build_root}/build
