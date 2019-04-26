/*
//@HEADER
// ************************************************************************
//
//                          features_defines.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_CONFIGS_FEATURES_FEATURES_DEFINES_H
#define INCLUDED_VT_CONFIGS_FEATURES_FEATURES_DEFINES_H

/*
 * All the defined features/options for debugging and backend enable-ifs
 */

// backend features, add any new ones to this list
#define vt_feature_no_feature         0 || vt_feature_cmake_no_feature
#define vt_feature_bit_check_overflow 0 || vt_feature_cmake_bit_check_overflo
#define vt_feature_trace_enabled      0 || vt_feature_cmake_trace_enabled
#define vt_feature_detector           0 || vt_feature_cmake_detector
#define vt_feature_lblite             0 || vt_feature_cmake_lblite
#define vt_feature_openmp             0 || vt_feature_cmake_openmp
#define vt_feature_production         0 || vt_feature_cmake_production
#define vt_feature_stdthread          0 || vt_feature_cmake_stdthread
#define vt_feature_mpi_rdma           0 || vt_feature_cmake_mpi_rdma
#define vt_feature_parserdes          0 || vt_feature_cmake_parserdes
#define vt_feature_print_term_msgs    0 || vt_feature_cmake_print_term_msgs
#define vt_feature_default_threading  0 || vt_feature_cmake_default_threading
#define vt_feature_no_pool_alloc_env  0 || vt_feature_cmake_no_pool_alloc_env
#define vt_feature_memory_pool        1 || vt_feature_cmake_memory_pool

#endif /*INCLUDED_VT_CONFIGS_FEATURES_FEATURES_DEFINES_H*/
