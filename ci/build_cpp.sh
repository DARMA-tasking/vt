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

if [ -z ${4} ]; then
    dashj=""
else
    dashj="-j ${4}"
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

# Match `nvcc_wrapper` and also a path ending with 'nvcc_wrapper'
case $CXX in
    *nvcc_wrapper)
        NVCC_WRAPPER_DEFAULT_COMPILER="$(which g++-"$(echo "${HOST_COMPILER}" | cut -d- -f2)")" \
        && export NVCC_WRAPPER_DEFAULT_COMPILER;;
esac

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
      -Dcheckpoint_DIR="$CHECKPOINT_BUILD/install" \
      -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-}" \
      -DCMAKE_INSTALL_PREFIX="$VT_BUILD/install" \
      -Dvt_ci_build="${VT_CI_BUILD:-0}" \
      -Dvt_debug_verbose="${VT_DEBUG_VERBOSE:-}" \
      -Dvt_tests_num_nodes="${VT_TESTS_NUM_NODES:-}" \
      -Dvt_no_color_enabled="${VT_NO_COLOR_ENABLED:-0}" \
      -DCMAKE_CXX_STANDARD="${CMAKE_CXX_STANDARD:-17}" \
      -DBUILD_SHARED_LIBS="${BUILD_SHARED_LIBS:-0}" \
      "$VT"
cmake_conf_ret=$?

if test "${VT_DOXYGEN_ENABLED:-0}" -eq 1
then
    MCSS=$PWD/m.css
    GHPAGE=$PWD/DARMA-tasking.github.io
    git clone "https://${token}@github.com/DARMA-tasking/DARMA-tasking.github.io"
    git clone https://github.com/mosra/m.css
    cd m.css
    git checkout master
    cd ../

    "$MCSS/documentation/doxygen.py" Doxyfile-mcss
    cp -R docs "$GHPAGE"
    cd "$GHPAGE"
    git config --global user.email "jliffla@sandia.gov"
    git config --global user.name "Jonathan Lifflander"
    git add docs
    git commit -m "Update docs (auto-build)"
    git push origin master
elif test "${VT_CI_BUILD:-0}" -eq 1
then
    # Generate output file with compilation warnings and errors

    GENERATOR=$(cmake -L . | grep USED_CMAKE_GENERATOR:STRING | cut -d"=" -f2)
    OUTPUT="$VT_BUILD"/compilation_errors_warnings.out
    OUTPUT_TMP="$OUTPUT".tmp

    # Because of the problem with new lines in Azure pipelines, all of them will be
    # converted to this unique delimiter
    DELIMITER="-=-=-=-"

    WARNS_ERRS=""

    # Unfortunately Ninja doesn't output compilation warnings and errors to stderr
    # so it needs special treatment
    if test "$GENERATOR" = "Ninja"
    then
        # To easily tell if compilation of given file succeeded special progress bar is used
        # (controlled by variable NINJA_STATUS)
        export NINJA_STATUS="[ninja][%f/%t] "
        time cmake --build . ${dashj} --target "${target}" | tee "$OUTPUT_TMP"
        compilation_ret=${PIPESTATUS[0]}
        sed -i '/ninja: build stopped:/d' "$OUTPUT_TMP"

        # Now every line that doesn't start with [ninja][number]/[number] is an error or a warning
        WARNS_ERRS=$(grep -Ev '^(\[ninja\]\[[[:digit:]]+\/[[:digit:]]+\])|(--) .*$' "$OUTPUT_TMP" || true)
    elif test "$GENERATOR" = "Unix Makefiles"
    then
        # Gcc outputs warnings and errors to stderr, so there's not much to do
        time cmake --build . ${dashj} --target "${target}" 2> >(tee "$OUTPUT_TMP")
        compilation_ret=$?
        WARNS_ERRS=$(cat "$OUTPUT_TMP")
    fi

    # Convert new lines and redirect to an output file
    WARNS_ERRS=${WARNS_ERRS//$'\n'/$DELIMITER}
    echo "$WARNS_ERRS" > "$OUTPUT"
else
    time cmake --build . ${dashj} --target "${target}"
    compilation_ret=$?
fi

if test "$use_ccache"
then
    { echo -e "===\n=== ccache statistics after build\n==="; } 2>/dev/null
    ccache -s
fi

# Exit with error code if there was any
if test "$cmake_conf_ret" -ne 0
then
    echo "There was an error during CMake configuration"
    exit "$cmake_conf_ret"
elif test "$compilation_ret" -ne 0
then
    echo "There was an error during compilation"
    exit "$compilation_ret"
fi
