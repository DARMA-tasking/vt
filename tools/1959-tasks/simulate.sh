#!/bin/bash

if [ -z "$2" ]; then
  echo "Error: missing arguments"
  echo "Syntax: $0 <vt_src_dir> <vt_build_dir>"
  exit 1
fi

vt_src_dir=$1
vt_build_dir=$2

mpiexec --n 14 ${vt_build_dir}/tools/workload_replay/simulate_replay 0 1 --vt_lb --vt_lb_file_name="${vt_src_dir}/tools/1959-tasks/ccm-lb-delta-1e-11.config" --vt_lb_data_in --vt_lb_data_dir_in="${vt_src_dir}/tools/1959-tasks" --vt_debug_level=terse --vt_debug_phase
