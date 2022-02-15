#!/usr/bin/env bash

set -exo pipefail

if test $# -lt 1
then
    echo "usage: ./$0 <libunwind-version>"
    exit 1
fi

libunwind_version=$1
libunwind_name="libunwind-${libunwind_version}"
libunwind_tar_name=${libunwind_name}.tar.gz

echo "${libunwind_version}"
echo "${libunwind_name}"
echo "${libunwind_tar_name}"

wget https://github.com/libunwind/libunwind/releases/download/v${libunwind_version}/${libunwind_tar_name}

tar xzf ${libunwind_tar_name}
rm ${libunwind_tar_name}
cd ${libunwind_name}
./configure \
    --enable-static \
    --enable-shared \
    --enable-setjmp=no \
    --prefix=`/usr/lib/x86_64-linux-gnu/`
make
make install
cd -
rm -rf ${libunwind_name}
