#!/usr/bin/env bash

set -exo pipefail

if test $# -lt 1
then
    echo "usage: ./$0 <ldms-version>"
    exit 1
fi

version=$1

echo "${version}"

git clone https://github.com/ovis-hpc/ovis.git
cd ovis
git checkout OVIS-${version}

./autogen.sh
./packaging/make-all-top.sh

cd -
