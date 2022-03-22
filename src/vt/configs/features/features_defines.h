/*
//@HEADER
// *****************************************************************************
//
//                              features_defines.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_CONFIGS_FEATURES_FEATURES_DEFINES_H
#define INCLUDED_VT_CONFIGS_FEATURES_FEATURES_DEFINES_H

#include <vt/cmake_config.h>

/*
 * All the defined features/options for debugging and backend enable-ifs
 */

// backend features, add any new ones to this list
#define vt_feature_no_feature          0 || vt_feature_cmake_no_feature
#define vt_feature_bit_check_overflow  0 || vt_feature_cmake_bit_check_overflo
#define vt_feature_trace_enabled       0 || vt_feature_cmake_trace_enabled
#define vt_feature_trace_only          0 || vt_feature_cmake_trace_only
#define vt_feature_lblite              0 || vt_feature_cmake_lblite
#define vt_feature_openmp              0 || vt_feature_cmake_openmp
#define vt_feature_production          0 || vt_feature_cmake_production
#define vt_feature_stdthread           0 || vt_feature_cmake_stdthread
#define vt_feature_mpi_rdma            0 || vt_feature_cmake_mpi_rdma
#define vt_feature_print_term_msgs     0 || vt_feature_cmake_print_term_msgs
#define vt_feature_no_pool_alloc_env   0 || vt_feature_cmake_no_pool_alloc_env
#define vt_feature_memory_pool         0 || vt_feature_cmake_memory_pool
#define vt_feature_priorities          0 || vt_feature_cmake_priorities
#define vt_feature_fcontext            0 || vt_feature_cmake_fcontext
#define vt_feature_mimalloc            0 || vt_feature_cmake_mimalloc
#define vt_feature_mpi_access_guards   0 || vt_feature_cmake_mpi_access_guards
#define vt_feature_zoltan              0 || vt_feature_cmake_zoltan
#define vt_feature_ci_build            0 || vt_feature_cmake_ci_build
#define vt_feature_diagnostics         0 || vt_feature_cmake_diagnostics
#define vt_feature_diagnostics_runtime 0 || vt_feature_cmake_diagnostics_runtime
#define vt_feature_libfort             0 || vt_feature_cmake_libfort
#define vt_feature_production_build    0 || vt_feature_cmake_production_build
#define vt_feature_debug_verbose       0 || vt_feature_cmake_debug_verbose

#define vt_check_enabled(test_option) (vt_feature_ ## test_option != 0)

#endif /*INCLUDED_VT_CONFIGS_FEATURES_FEATURES_DEFINES_H*/
