#!/usr/bin/env bash

if test $# -ne 1
then
    echo "usage $0 <build-mode>"
    exit 1;
fi

BUILD_MODE=$1

echo "Building virtual transport layer mode=$BUILD_MODE"

SOURCE_BASE_DIR=../virtual-transport/
MAKE_VERBOSE=off

CXX_COMPILER=mpic++-mpich-clang #mpic++-mpich-gcc5
CC_COMPILER=mpicc-mpich-clang #mpicc-mpich-gcc5

cmake ${SOURCE_BASE_DIR} \
      -DCMAKE_INSTALL_PREFIX=`pwd` \
      -DCMAKE_CXX_FLAGS="-std=c++1y" \
      -DCMAKE_BUILD_TYPE=${BUILD_MODE} \
      -DCMAKE_VERBOSE_MAKEFILE:BOOL=$MAKE_VERBOSE \
      -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
      -DCMAKE_C_COMPILER=${CC_COMPILER}
