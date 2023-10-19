#!/usr/bin/env bash

set -exo pipefail

if test $# -lt 2
then
    echo "usage: ./$0 <vtk-version> <installation-dir>"
    exit 1
fi

vtk_version="$1"
vtk_tar_name="v${vtk_version}.tar.gz"
vtk_name="VTK-${vtk_version}"
install_dir="${2-}"
build_dir=/vtk-build/

wget "https://github.com/Kitware/VTK/archive/refs/tags/$vtk_tar_name"
tar xzf ${vtk_tar_name}
rm ${vtk_tar_name}
cd ${vtk_name}

mkdir -p ${build_dir}
mkdir -p ${install_dir}

cd ${build_dir}
cmake \
  -DCMAKE_INSTALL_PREFIX:FILEPATH=${install_dir} \
  ../${vtk_name}
cmake --build ${build_dir} --target install
