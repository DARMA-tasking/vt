#!/usr/bin/env bash

set -ex

source_dir=${1}
build_dir=${2}

# Dependency versions, when fetched via git.
checkpoint_rev=develop

if test "${VT_DOXYGEN_ENABLED:-0}" -eq 1
then
    token=${3}
else
    target=${3:-install}
fi

    export parallel_level=4
if [ -z ${4} ]; then
    export dashj=""
else
    export parallel_level=${4}
    export dashj="-j ${4}"
fi

if hash ccache &>/dev/null
then
    use_ccache=true
fi

if test "$use_ccache"
then
    { echo -e "===\n=== ccache statistics before build\n==="; } 2>/dev/null
    ccache -s
else
    { echo -e "===\n=== ccache not found, compiling without it\n==="; } 2>/dev/null
fi

mkdir -p "${build_dir}"
pushd "${build_dir}"

if test -d "checkpoint"
then
    rm -Rf checkpoint
fi

if test -d "${source_dir}/lib/checkpoint"
then
    { echo "Checkpoint already in lib... not downloading, building, and installing"; } 2>/dev/null
else
    if test "${VT_DOXYGEN_ENABLED:-0}" -eq 1
    then
        cd "${source_dir}/lib"
        git clone -b "${checkpoint_rev}" --depth 1 https://github.com/DARMA-tasking/checkpoint.git
        cd -
    else
        git clone -b "${checkpoint_rev}" --depth 1 https://github.com/DARMA-tasking/checkpoint.git
        export CHECKPOINT=$PWD/checkpoint
        export CHECKPOINT_BUILD=${build_dir}/checkpoint
        mkdir -p "$CHECKPOINT_BUILD"
        cd "$CHECKPOINT_BUILD"
        mkdir build
        cd build
        cmake -G "${CMAKE_GENERATOR:-Ninja}" \
              -DCMAKE_INSTALL_PREFIX="$CHECKPOINT_BUILD/install" \
              "$CHECKPOINT"
        cmake --build . ${dashj} --target install
    fi
fi

if test "${VT_ZOLTAN_ENABLED:-0}" -eq 1
then
    export Zoltan_DIR=${ZOLTAN_DIR:-""}
fi

if test "${VT_CI_BUILD:-0}" -eq 1
then
    git config --global --add safe.directory "${source_dir}"
fi

export VT=${source_dir}
export VT_BUILD=${build_dir}/vt
mkdir -p "$VT_BUILD"
cd "$VT_BUILD"
rm -Rf ./*

ctest -S $VT/ci/ctest_job_script.cmake

ctest_ret=$?

# Exit with error code if there was any
if test "$ctest_ret" -ne 0
then
    echo "There was an error during CTest"
    exit "$ctest_ret"
fi
