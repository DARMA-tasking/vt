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
# echo "${mpich_version}"
# echo "${mpich_name}"
# echo "${mpich_tar_name}"
# echo "${make_flags}"

# wget http://www.mpich.org/static/downloads/${mpich_version}/${mpich_tar_name}
# tar xzf ${mpich_tar_name}
# rm ${mpich_tar_name}
# cd ${mpich_name}
# ./configure \
#     --enable-static=false \
#     --enable-alloca=true \
#     --disable-long-double \
#     --enable-threads=single \
#     --enable-fortran=no \
#     --enable-fast=all \
#     --enable-g=none \
#     --enable-timing=none \
#     ${installation_prefix:+ --prefix"=${installation_prefix}"}
# make ${make_flags}
# make install
# cd -
# rm -rf ${mpich_name}


# # Clone VTK source
# RUN mkdir -p /opt/src/vtk
# RUN git clone --recursive --branch v9.2.2 https://gitlab.kitware.com/vtk/vtk.git /opt/src/vtk

# # Build VTK
# RUN mkdir -p /opt/build/vtk-build
# WORKDIR /opt/build/vtk-build
# RUN cmake \
#   -DCMAKE_C_COMPILER=gcc-11 \
#   -DCMAKE_CXX_COMPILER=g++-11 \
#   -DCMAKE_BUILD_TYPE:STRING=Release \
#   -DBUILD_TESTING:BOOL=OFF \
#   -DVTK_Group_Rendering:BOOL=OFF \
#   -DBUILD_TESTING:BOOL=OFF \
#   -DBUILD_SHARED_LIBS:BOOL=ON \
#   -S /opt/src/vtk -B /opt/build/vtk-build
# RUN cmake --build /opt/build/vtk-build -j$(nproc)
