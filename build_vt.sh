#!/usr/bin/env bash

if test $# -lt 1
then
    echo "usage $0 <build-mode> <compiler>"
    exit 1;
fi

build_mode=$1

if test $# -gt 1
then
    compiler=$2
else
    compiler="clang"
fi

echo "Building virtual transport layer mode=$build_mode"

SOURCE_BASE_DIR=../virtual-transport/
MAKE_VERBOSE=off

if test $compiler = "clang"
then
    CXX_COMPILER=clang++-mp-3.9 # mpic++-mpich-clang39
    CC_COMPILER=clang-mp-3.9 #mpicc-mpich-clang39
else
    CXX_COMPILER=mpic++-mpich-devel-gcc6
    CC_COMPILER=mpicc-mpich-devel-gcc6
fi

MPI_PATH=/opt/local/lib/mpich-clang39/libmpi.dylib
MPI_INC_PATH=/opt/local/include/mpich-clang39/
MPI_CXX_PATH=/opt/local/lib/mpich-clang39/libmpicxx.dylib
MPI_CXX_INC_PATH=/opt/local/include/mpich-clang39/mpicxx.h

gtest_directory=/Users/jliffla/codes/gtest/gtest-build

cmake ${SOURCE_BASE_DIR} \
      -DCMAKE_INSTALL_PREFIX=`pwd` \
      -DCMAKE_CXX_FLAGS="-std=c++1y" \
      -DCMAKE_BUILD_TYPE=${build_mode} \
      -DCMAKE_VERBOSE_MAKEFILE:BOOL=$MAKE_VERBOSE \
      -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
      -DMPI_C_LIBRARIES=${MPI_PATH} \
      -DMPI_C_INCLUDE_PATH=${MPI_INC_PATH} \
      -DMPI_CXX_LIBRARIES=${MPI_CXX_PATH} \
      -DMPI_CXX_INCLUDE_PATH=${MPI_CXX_INC_PATH} \
      -DGTEST_DIR=${gtest_directory} \
      -DCMAKE_C_COMPILER=${CC_COMPILER}


