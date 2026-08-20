#!/usr/bin/env bash

set -exo pipefail

cur_path=$(pwd)
vt_spack_package="$cur_path/spack-package"

# DARMA-tasking/spack-package uses Spack's v2 repository API, including the
# spack_repo.builtin build-system imports introduced in Spack 1.x.
git clone --branch v1.0.4 --depth=2 https://github.com/spack/spack.git
. spack/share/spack/setup-env.sh

git clone -b master https://github.com/DARMA-tasking/spack-package.git
python3 spack-package/ci/add_vt_branch.py "${GIT_BRANCH}"

declare -A variables_map
variables_map["lb_enabled"]="${VT_LB_ENABLED:-0}"
variables_map["trace_enabled"]="${VT_TRACE_ENABLED:-0}"
variables_map["trace_only"]="${VT_BUILD_TRACE_ONLY:-1}"
variables_map["mimalloc_enabled"]="${VT_MIMALLOC_ENABLED:-0}"
variables_map["asan_enabled"]="${VT_ASAN_ENABLED:-0}"
variables_map["werror_enabled"]="${VT_WERROR_ENABLED:-0}"
variables_map["pool_enabled"]="${VT_POOL_ENABLED:-0}"
variables_map["mpi_guards"]="${VT_MPI_GUARD_ENABLED:-0}"
variables_map["kokkos"]="${VT_KOKKOS_ENABLED:-0}"

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
install_cmd="spack install darma-vt@${GIT_BRANCH} build_type=Release ${install_cmd:1} ^openmpi@4.0.4"

mkdir -p ~/.spack
cat >> ~/.spack/packages.yaml <<'EOF'
packages:
  openmpi:
    externals:
    - spec: openmpi@4.0.4
      prefix: /usr/local
EOF

spack clean --all
spack repo add "$vt_spack_package"
spack external find

$install_cmd

git clone https://github.com/DARMA-tasking/vt-sample-project
mkdir -p vt-sample-project/build
cd vt-sample-project/build || exit 1
vt_DIR=$(spack location --install-dir darma-vt)
export vt_DIR
cmake -G "${CMAKE_GENERATOR:-Ninja}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER="${CXX:-c++}" \
  -DCMAKE_C_COMPILER="${CC:-cc}" \
  -Dbuild_with_libs=1 \
  ..
cmake --build .
