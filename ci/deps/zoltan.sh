#!/usr/bin/env bash

set -exo pipefail

if test $# -lt 2
then
    echo "usage: ./$0 <make_flags> <install-dir>"
    exit 1
fi

make_flags=$1
build_dir=/trilinos-build/
install_dir=$2

if test -d Trilinos
then
    echo "Found Trilinos already"
else
    git clone https://github.com/trilinos/Trilinos.git --depth=1
fi

mkdir -p ${build_dir}
mkdir -p ${install_dir}

cd ${build_dir}
export FC=/usr/bin/gfortran
cmake \
  -DCMAKE_INSTALL_PREFIX:FILEPATH=${install_dir} \
  -DTPL_ENABLE_MPI:BOOL=ON \
  -DCMAKE_C_FLAGS:STRING="-m64 -g" \
  -DCMAKE_CXX_FLAGS:STRING="-m64 -g" \
  -DTrilinos_ENABLE_ALL_PACKAGES:BOOL=OFF \
  -DTrilinos_ENABLE_Zoltan:BOOL=ON \
  -DZoltan_ENABLE_ULLONG_IDS:Bool=ON \
  ../Trilinos
make ${make_flags}
make install
cd -
rm -rf Trilinos
rm -rf ${build_dir}
