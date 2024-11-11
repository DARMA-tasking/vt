#!/usr/bin/env bash

set -exo pipefail

if test $# -ne 1
then
    echo "usage: ./$0 <doxygen-version>"
    exit 1
fi

doxygen_version=$1
doxygen_tar_name=doxygen-"${doxygen_version}".linux.bin.tar.gz

echo "${doxygen_tar_name}"

wget https://sourceforge.net/projects/doxygen/files/rel-"${doxygen_version}"/"${doxygen_tar_name}"

tar xzf "${doxygen_tar_name}" --one-top-level=doxygen --strip-components=1
