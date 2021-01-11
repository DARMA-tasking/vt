#!/usr/bin/env bash

set -ex

dir=$(pwd)

if test -d Trilinos
then
    echo "Found Trilinos already"
else
    git clone https://github.com/trilinos/Trilinos.git --depth=1
    mkdir -p trilinos-{build,install}
fi

cd trilinos-build
rm -rf ./*

# -D Zoltan_ENABLE_EXAMPLES:BOOL=ON \
# -D Zoltan_ENABLE_TESTS:BOOL=ON \

cmake \
    -D CMAKE_INSTALL_PREFIX:FILEPATH="${dir}/trilinos-install/" \
    -D TPL_ENABLE_MPI:BOOL=ON \
    -D CMAKE_C_FLAGS:STRING="-m64 -g" \
    -D CMAKE_CXX_FLAGS:STRING="-m64 -g" \
    -D Trilinos_ENABLE_ALL_PACKAGES:BOOL=OFF \
    -D Trilinos_ENABLE_Zoltan:BOOL=ON \
    -D Zoltan_ENABLE_ULLONG_IDS:Bool=ON \
    ../Trilinos
make -j4
make install
