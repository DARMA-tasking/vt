#!/usr/bin/env bash

cur_path=$(pwd)
spack_path="$cur_path/spack"
vt_spack_package="$cur_path/spack-package"

git clone https://github.com/spack/spack.git
git clone https://github.com/DARMA-tasking/spack-package.git

cd "$spack_path" || exit 1
git checkout v0.16.3
cd "$cur_path" || exit 1

declare -A variables_map
variables_map["lb_enabled"]="${VT_LB_ENABLED:-0}"
variables_map["trace_enabled"]="${VT_TRACE_ENABLED:-0}"
variables_map["trace_only"]="${VT_BUILD_TRACE_ONLY:-0}"
variables_map["doxygen_enabled"]="${VT_DOXYGEN_ENABLED:-0}"
variables_map["mimalloc_enabled"]="${VT_MIMALLOC_ENABLED:-0}"
variables_map["asan_enabled"]="${VT_ASAN_ENABLED:-0}"
variables_map["werror_enabled"]="${VT_WERROR_ENABLED:-0}"
variables_map["pool_enabled"]="${VT_POOL_ENABLED:-0}"
variables_map["zoltan_enabled"]="${VT_ZOLTAN_ENABLED:-0}"
variables_map["mpi_guards"]="${VT_MPI_GUARD_ENABLED:-0}"
variables_map["diagnostics_enabled"]="${VT_DIAGNOSTICS_ENABLED:-0}"
variables_map["diagnostics_runtime_enabled"]="${VT_DIAGNOSTICS_RUNTIME_ENABLED:-0}"
variables_map["unity_build_enabled"]="${VT_UNITY_BUILD_ENABLED:-0}"
variables_map["fcontext_enabled"]="${VT_FCONTEXT_ENABLED:-0}"
variables_map["use_openmp"]="${VT_USE_OPENMP:-0}"
variables_map["use_std_thread"]="${VT_USE_STD_THREAD:-0}"

cmd_vars=()
for flag in "${!variables_map[@]}"
do
  flag_var=${variables_map[${flag}]}
  if test "$flag_var" -eq 0
  then
    cmd_vars+=("-$flag")
  else
    cmd_vars+=("+$flag")
  fi
done

install_cmd=$(printf " %s" "${cmd_vars[@]}")
install_cmd="$spack_path/bin/spack install darma-vt@develop build_type=Release ${install_cmd:1}"

"$spack_path"/bin/spack repo add "$vt_spack_package"
"$spack_path"/bin/spack external find
$install_cmd

git clone https://github.com/DARMA-tasking/vt-sample-project
mkdir -p vt-sample-project/build
cd vt-sample-project/build || exit 1
vt_DIR=$("$spack_path"/bin/spack location --install-dir darma-vt)
export vt_DIR
cmake -G "${CMAKE_GENERATOR:-Ninja}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER="${CXX:-c++}" \
  -DCMAKE_C_COMPILER="${CC:-cc}" \
  -Dbuild_with_libs=1 \
  ..
cmake --build .
