#!/usr/bin/env bash

set -ex

source_dir=${1}
build_dir=${2}

inclusion_type=""
if test "$VT_INCLUSION_TYPE" = "TPL"
then
  inclusion_type="-Dbuild_with_tpl=1"
elif test "$VT_INCLUSION_TYPE" = "EXT_LIB"
then
  inclusion_type="-Dbuild_with_libs=1"
fi

# Don't build vt-sample-project on Alpine Linux
is_alpine="$(grep ID < /etc/os-release | grep -c alpine || true)"
if test "$is_alpine" -eq 0 && test "${VT_CI_BUILD:-0}" -eq 1
then
    export VT=${source_dir}
    export VT_BUILD=${build_dir}/vt
    export VT_INSTALL=${VT_BUILD}/install/cmake
    export CHECKPOINT=${build_dir}/checkpoint

    cd "$VT_BUILD"

    if test "$VT_INCLUSION_TYPE" = "TPL"
    then
        echo "Clean up before building vt-sample-project"
        cmake --build . --target clean
        rm -rf "$VT_INSTALL"
    fi

    if test "$VT_INCLUSION_TYPE" = "EXT_LIB"
    then
        VT="$VT_INSTALL"
        CHECKPOINT=""
    fi

    git clone https://github.com/DARMA-tasking/vt-sample-project
    mkdir -p vt-sample-project/build
    cd vt-sample-project/build || exit

    cmake -G "${CMAKE_GENERATOR:-Ninja}" \
      -Dvt_DIR="${VT}" \
      -Dcheckpoint_DIR="${CHECKPOINT}" \
      -Dkokkos_DISABLE:BOOL=1 \
      -Dkokkos_kernels_DISABLE:BOOL=1 \
      -Dvt_trace_only="1" \
      -DVT_BUILD_EXAMPLES="0" \
      -DVT_BUILD_TESTS="0" \
      -DCMAKE_CXX_STANDARD="${CMAKE_CXX_STANDARD:-17}" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
      -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}" \
      -DCMAKE_CXX_COMPILER="${CXX:-c++}" \
      -DCMAKE_C_COMPILER="${CC:-cc}" \
      "$inclusion_type" \
      ..
    cmake --build .

    # Try to actually run samples
    mpiexec -n 2 ./vt-runtime-sample
    mpiexec -n 2 ./vt-trace-only-sample
fi
