#!/usr/bin/env bash

set -ex

source_dir=${1}
build_dir=${2}

# Dependency versions, when fetched via git.
detector_rev=master
checkpoint_rev=develop

if [ -z ${4} ]; then
    dashj=""
else
    dashj="-j ${4}"
fi

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
    { echo "Detector already in lib... not downloading, building, and installing"; } 2>/dev/null
else
    if test "${VT_DOXYGEN_ENABLED:-0}" -eq 1
    then
        cd "${source_dir}/lib"
        git clone -b "${detector_rev}" --depth 1 https://github.com/DARMA-tasking/detector.git
        cd -
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
      cmake --build . ${dashj} --target install
    fi
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
              -Ddetector_DIR="$DETECTOR_BUILD/install" \
              "$CHECKPOINT"
        cmake --build . ${dashj} --target install
    fi
fi

if test "${VT_ZOLTAN_ENABLED:-0}" -eq 1
then
    export Zoltan_DIR=${ZOLTAN_DIR:-""}
fi

export VT=${source_dir}
export VT_BUILD=${build_dir}/vt
mkdir -p "$VT_BUILD"
cd "$VT_BUILD"
rm -Rf ./*
cmake -G "${CMAKE_GENERATOR:-Ninja}" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
      -Dvt_test_trace_runtime_enabled="${VT_TRACE_RUNTIME_ENABLED:-0}" \
      -Dvt_lb_enabled="${VT_LB_ENABLED:-1}" \
      -Dvt_trace_enabled="${VT_TRACE_ENABLED:-0}" \
      -Dvt_trace_only="${VT_BUILD_TRACE_ONLY:-0}" \
      -Dvt_doxygen_enabled="${VT_DOXYGEN_ENABLED:-0}" \
      -Dvt_mimalloc_enabled="${VT_MIMALLOC_ENABLED:-0}" \
      -Dvt_asan_enabled="${VT_ASAN_ENABLED:-0}" \
      -Dvt_ubsan_enabled="${VT_UBSAN_ENABLED:-0}" \
      -Dvt_werror_enabled="${VT_WERROR_ENABLED:-0}" \
      -Dvt_pool_enabled="${VT_POOL_ENABLED:-1}" \
      -Dvt_build_extended_tests="${VT_EXTENDED_TESTS_ENABLED:-1}" \
      -Dvt_zoltan_enabled="${VT_ZOLTAN_ENABLED:-0}" \
      -Dvt_production_build_enabled="${VT_PRODUCTION_BUILD_ENABLED:-0}" \
      -Dvt_unity_build_enabled="${VT_UNITY_BUILD_ENABLED:-0}" \
      -Dvt_diagnostics_enabled="${VT_DIAGNOSTICS_ENABLED:-1}" \
      -Dvt_diagnostics_runtime_enabled="${VT_DIAGNOSTICS_RUNTIME_ENABLED:-0}" \
      -Dvt_fcontext_enabled="${VT_FCONTEXT_ENABLED:-0}" \
      -Dvt_fcontext_build_tests_examples="${VT_FCONTEXT_BUILD_TESTS_EXAMPLES:-0}" \
      -Dvt_rdma_tests_enabled="${VT_RDMA_TESTS_ENABLED:-1}" \
      -DUSE_OPENMP="${VT_USE_OPENMP:-0}" \
      -DUSE_STD_THREAD="${VT_USE_STD_THREAD:-0}" \
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
      -Dvt_debug_verbose="${VT_DEBUG_VERBOSE:-}" \
      -Dvt_tests_num_nodes="${VT_TESTS_NUM_NODES:-}" \
      -Dvt_no_color_enabled="${VT_NO_COLOR_ENABLED:-0}" \
      -DBUILD_SHARED_LIBS="${BUILD_SHARED_LIBS:-0}" \
      "$VT"

run-clang-tidy -checks='-*, bugprone-*'
