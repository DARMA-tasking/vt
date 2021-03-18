#!/usr/bin/env bash

cur_path=$(pwd)
spack_path="$cur_path/spack"
vt_spack_package="$cur_path/spack-package"

git clone https://github.com/spack/spack.git
git clone https://github.com/DARMA-tasking/spack-package.git

"$spack_path"/bin/spack repo add "$vt_spack_package"
"$spack_path"/bin/spack external find
"$spack_path"/bin/spack install darma-vt@develop \
    build_type=Release \
    lb_enabled="$VT_LB_ENABLED" \
    trace_enabled="$VT_TRACE_ENABLED" \
    trace_only="$VT_BUILD_TRACE_ONLY" \
    doxygen_enabled="$VT_DOXYGEN_ENABLED" \
    mimalloc_enabled="$VT_MIMALLOC_ENABLED" \
    asan_enabled="$VT_ASAN_ENABLED" \
    werror_enabled="$VT_WERROR_ENABLED" \
    pool_enabled="$VT_POOL_ENABLED" \
    zoltan_enabled="$VT_ZOLTAN_ENABLED" \
    mpi_guards="$VT_MPI_GUARD_ENABLED" \
    diagnostics_enabled="$VT_DIAGNOSTICS_ENABLED" \
    diagnostics_runtime_enabled="$VT_DIAGNOSTICS_RUNTIME_ENABLED" \
    unity_build_enabled="$VT_UNITY_BUILD_ENABLED" \
    fcontext_enabled="$VT_FCONTEXT_ENABLED" \
    use_openmp="$VT_USE_OPENMP" \
    use_std_thread="$VT_USE_STD_THREAD" \
    -build_tests \
    -build_examples

git clone https://github.com/DARMA-tasking/vt-sample-project
mkdir -p vt-sample-project/build
cd vt-sample-project/build || exit 1
vt_DIR=$("$spack_path"/bin/spack location --install-dir darma-vt)
export vt_DIR
cmake -G "${CMAKE_GENERATOR:-Ninja}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER="${CXX:-c++}" \
  -DCMAKE_C_COMPILER="${CC:-cc}" \
  ..
cmake --build .
