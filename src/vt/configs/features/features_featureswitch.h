/*
//@HEADER
// *****************************************************************************
//
//                           features_featureswitch.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_CONFIGS_FEATURES_FEATURES_FEATURESWITCH_H
#define INCLUDED_VT_CONFIGS_FEATURES_FEATURES_FEATURESWITCH_H

/*
 * Strings for various vt features
 */

#define vt_feature_str_bit_check_overflow "Check bitfield overflow"
#define vt_feature_str_cons_multi_idx     "Collection Constructor Positional"
#define vt_feature_str_detector           "C++ Trait Detector"
#define vt_feature_str_diagnostics        "Performance analysis diagnostics"
#define vt_feature_str_fcontext           "User-level threading with fcontext"
#define vt_feature_str_lblite             "Load Balancing for Collections"
#define vt_feature_str_memory_pool        "Memory Pooling"
#define vt_feature_str_mimalloc           "mimalloc memory allocator"
#define vt_feature_str_mpi_access_guards  "MPI access guards"
#define vt_feature_str_mpi_rdma           "Native RDMA with MPI"
#define vt_feature_str_no_feature         "No feature"
#define vt_feature_str_no_pool_alloc_env  "No memory pool envelope"
#define vt_feature_str_openmp             "OpenMP Threading"
#define vt_feature_str_print_term_msgs    "Print Termination Control Messages"
#define vt_feature_str_priorities         "Message priorities"
#define vt_feature_str_production_build   "Production Build (assertions and " \
                                          "debug prints disabled)"
#define vt_feature_str_stdthread          "std::thread Threading"
#define vt_feature_str_trace_enabled      "Tracing Projections"
#define vt_feature_str_zoltan             "Zoltan for load balancing"

#endif /*INCLUDED_VT_CONFIGS_FEATURES_FEATURES_FEATURESWITCH_H*/
