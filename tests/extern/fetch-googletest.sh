#! /bin/sh

GTEST_BRANCHTAG=release-1.12.1

# Fetch googletest from Github.
# GitHub does not support git-archive - however this endpoint works (MAR 2020)
mkdir -p googletest && rm -fr googletest/*
curl -L https://github.com/google/googletest/archive/refs/tags/$GTEST_BRANCHTAG.tar.gz \
   | tar zxf - -C googletest --strip-components 1

# Remove googletest artifacts - set cmake BUILD_GMOCK=0, don't install, don't build test.
pushd googletest
rm -fr .[!.]* BUILD.bazel WORKSPACE appveyor.yml library.json platformio.ini
rm -fr ci/
rm -fr googlemock/
rm -fr googletest/test/ googletest/samples/ googletest/docs/ googletest/scripts/
rm -fr docs
popd
