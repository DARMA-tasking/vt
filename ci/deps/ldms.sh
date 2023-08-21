#!/usr/bin/env bash

set -exo pipefail

if test $# -lt 1
then
    echo "usage: ./$0 <ldms-version>"
    exit 1
fi

version=$1
arch=x86_64

ldms_tar_name=ovis-ldms-${version}.tar.gz
ldms_name=ovis-ldms-${version}

echo "${version}"
echo "${ldms_tar_name}"

wget https://github.com/ovis-hpc/ovis/releases/download/OVIS-${version}/${ldms_tar_name}

tar xzf ${ldms_tar_name}
rm ${ldms_tar_name}
cd ${ldms_name}
./configure \
    ${installation_prefix:+ --prefix"=${installation_prefix}"}
make -j4
make install
cd -
rm -rf ${ldms_name}

