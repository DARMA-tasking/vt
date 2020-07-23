#!/usr/bin/env bash

set -exo pipefail

if test $# -lt 3
then
    echo "usage: ./$0 <openmpi-dir> <openmpi-version> <make_flags>"
    exit 1
fi

openmpi_dir=$1
openmpi_version=$2
openmpi_name="openmpi-${openmpi_version}"
openmpi_tar_name="${openmpi_name}.tar.gz"
make_flags="$3"

echo "${openmpi_dir}"
echo "${openmpi_version}"
echo "${openmpi_name}"
echo "${openmpi_tar_name}"
echo "${make_flags}"

wget https://download.open-mpi.org/release/open-mpi/${openmpi_dir}/${openmpi_tar_name}
tar xzf ${openmpi_tar_name}
rm ${openmpi_tar_name}
cd ${openmpi_name}
./configure \
    --disable-shared \
    --enable-static
make ${make_flags}
make install
cd -
rm -rf ${openmpi_name}
