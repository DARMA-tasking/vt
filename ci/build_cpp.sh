#!/usr/bin/env bash

set -ex

source_dir=${1}
build_dir=${2}

# Dependency versions, when fetched via git.
detector_rev=master
checkpoint_rev=156-footprint-non-serializable-elements

if test "${VT_DOXYGEN_ENABLED:-0}" -eq 1
then
    token=${3}
else
    target=${3:-install}
fi

echo -e "===\n=== ccache statistics before build\n==="
ccache -s

mkdir -p "${build_dir}"
pushd "${build_dir}"

if test -d "detector"
then
    rm -Rf detector
fi

if test -d "checkpoint"
then
    rm -Rf checkpoint
fi

if test -d "${source_dir}/lib/detector"
then
    echo "Detector already in lib... not downloading, building, and installing"
else
    git clone -b "${detector_rev}" --depth 1 https://github.com/DARMA-tasking/detector.git
    export DETECTOR=$PWD/detector
    export DETECTOR_BUILD=${build_dir}/detector
    mkdir -p "$DETECTOR_BUILD"
    cd "$DETECTOR_BUILD"
    mkdir build
    cd build
    cmake -G "${CMAKE_GENERATOR:-Ninja}" \
          -DCMAKE_INSTALL_PREFIX="$DETECTOR_BUILD/install" \
          "$DETECTOR"
    cmake --build . --target install
fi

if test -d "${source_dir}/lib/checkpoint"
then
    echo "Checkpoint already in lib... not downloading, building, and installing"
else
    if test "${VT_DOXYGEN_ENABLED:-0}" -eq 1
    then
        cd "${source_dir}/lib"
        git clone -b "${checkpoint_rev}" --depth 1 https://github.com/DARMA-tasking/checkpoint.git
        cd ../../
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
              -Ddetector_DIR="$DETECTOR_BUILD/install" \
              "$CHECKPOINT"
        cmake --build . --target install
    fi
fi

if test ${VT_ZOLTAN_ENABLED:-0} -eq 1
then
    export ZOLTAN_CONFIG=${ZOLTAN_DIR:-""}
fi

export VT=${source_dir}
export VT_BUILD=${build_dir}/vt
mkdir -p "$VT_BUILD"
cd "$VT_BUILD"
rm -Rf ./*
cmake -G "${CMAKE_GENERATOR:-Ninja}" \
      -Dvt_test_trace_runtime_enabled="${VT_TRACE_RUNTIME_ENABLED:-0}" \
      -Dvt_lb_enabled="${VT_LB_ENABLED:-1}" \
      -Dvt_trace_enabled="${VT_TRACE_ENABLED:-0}" \
      -Dvt_doxygen_enabled="${VT_DOXYGEN_ENABLED:-0}" \
      -Dvt_mimalloc_enabled="${VT_MIMALLOC_ENABLED:-0}" \
      -Dvt_asan_enabled="${VT_ASAN_ENABLED:-0}" \
      -Dvt_pool_enabled="${VT_POOL_ENABLED:-1}" \
      -Dvt_build_extended_tests="${VT_EXTENDED_TESTS_ENABLED:-1}" \
      -Dvt_zoltan_enabled="${VT_ZOLTAN_ENABLED:-0}" \
      -Dvt_unity_build_enabled="${VT_UNITY_BUILD_ENABLED:-0}" \
      -Dvt_diagnostics_enabled="${VT_DIAGNOSTICS_ENABLED:-1}" \
      -Dvt_diagnostics_runtime_enabled="${VT_DIAGNOSTICS_RUNTIME_ENABLED:-0}" \
      -Dzoltan_DIR="${ZOLTAN_CONFIG:-}" \
      -DCODE_COVERAGE="${CODE_COVERAGE:-0}" \
      -DMI_INTERPOSE:BOOL=ON \
      -DMI_OVERRIDE:BOOL=ON \
      -Dvt_mpi_guards="${VT_MPI_GUARD_ENABLED:-0}" \
      -DMPI_EXTRA_FLAGS="${MPI_EXTRA_FLAGS:-}" \
      -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}" \
      -DMPI_C_COMPILER="${MPICC:-mpicc}" \
      -DMPI_CXX_COMPILER="${MPICXX:-mpicxx}" \
      -DCMAKE_CXX_COMPILER="${CXX:-c++}" \
      -DCMAKE_C_COMPILER="${CC:-cc}" \
      -DCMAKE_EXE_LINKER_FLAGS="${CMAKE_EXE_LINKER_FLAGS:-}" \
      -Ddetector_DIR="$DETECTOR_BUILD/install" \
      -Dcheckpoint_DIR="$CHECKPOINT_BUILD/install" \
      -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-}" \
      -DCMAKE_INSTALL_PREFIX="$VT_BUILD/install" \
      -Dvt_ci_build="${VT_CI_BUILD:-0}" \
      "$VT"

if test "${VT_DOXYGEN_ENABLED:-0}" -eq 1
then
    MCSS=$PWD/m.css
    GHPAGE=$PWD/DARMA-tasking.github.io
    git clone "https://${token}@github.com/DARMA-tasking/DARMA-tasking.github.io"
    git clone https://github.com/mosra/m.css
    cd m.css
    git checkout 6eefd92c2aa3e0a257503d31b1a469867dfff8b6
    cd ../

    "$MCSS/documentation/doxygen.py" Doxyfile-mcss
    cp  -R docs "$GHPAGE"
    cd "$GHPAGE"
    git config --global user.email "jliffla@sandia.gov"
    git config --global user.name "Jonathan Lifflander"
    git add docs
    git commit -m "Update docs (auto-build)"
    git push origin master
else
    time cmake --build . --target "${target}"
fi

echo -e "===\n=== ccache statistics after build\n==="
ccache -s
