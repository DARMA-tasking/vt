/*
//@HEADER
// ************************************************************************
//
//                          features_featureswitch.h
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

#if !defined INCLUDED_VT_CONFIGS_FEATURES_FEATURES_FEATURESWITCH_H
#define INCLUDED_VT_CONFIGS_FEATURES_FEATURES_FEATURESWITCH_H

#include "vt/configs/features/features_defines.h"

/*
 * Strings for various vt features
 */

#define vt_feature_str_bit_check_overflow "Check bitfield overflow"
#define vt_feature_str_detector           "C++ Trait Detector"
#define vt_feature_str_lblite             "Load Balancing for Collections"
#define vt_feature_str_memory_pool        "Memory Pooling"
#define vt_feature_str_mpi_rdma           "Native RDMA with MPI"
#define vt_feature_str_no_feature         "No feature"
#define vt_feature_str_no_pool_alloc_env  "No memory pool envelope"
#define vt_feature_str_openmp             "OpenMP Threading"
#define vt_feature_str_parserdes          "Partial Inline Serialization"
#define vt_feature_str_print_term_msgs    "Print Termination Control Messages"
#define vt_feature_str_production         "Production Build"
#define vt_feature_str_stdthread          "std::thread Threading"
#define vt_feature_str_trace_enabled      "Tracing Projections"
#define vt_feature_str_cons_multi_idx     "Collection Constructor Positional"

#endif /*INCLUDED_VT_CONFIGS_FEATURES_FEATURES_FEATURESWITCH_H*/
