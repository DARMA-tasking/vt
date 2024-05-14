#!/usr/bin/env bash

set -exo pipefail

if test $# -lt 2
then
    echo "usage: ./$0 <mpich-version> <make_flags> <installation_prefix>"
    exit 1
fi

mpich_version=$1
mpich_name="mpich-${mpich_version}"
mpich_tar_name="${mpich_name}.tar.gz"
make_flags="$2"
installation_prefix="${3-}"

echo "${mpich_version}"
echo "${mpich_name}"
echo "${mpich_tar_name}"
echo "${make_flags}"

wget http://www.mpich.org/static/downloads/${mpich_version}/${mpich_tar_name}
tar xzf ${mpich_tar_name}
rm ${mpich_tar_name}
cd ${mpich_name}
./configure \
    --enable-static=false \
    --enable-alloca=true \
    --disable-long-double \
    --enable-threads=single \
    --enable-fortran=no \
    --enable-fast=all \
    --enable-g=none \
    --enable-timing=none \
    ${installation_prefix:+ --prefix"=${installation_prefix}"}
make ${make_flags}
make install
cd -
rm -rf ${mpich_name}
