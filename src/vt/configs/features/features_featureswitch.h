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

#if !defined INCLUDED_FEATURES_FEATURE_SWITCH
#define INCLUDED_FEATURES_FEATURE_SWITCH

#include "meld_headers.h"
#include "vt/configs/features/features_defines.h"

/*
 * Key-value maps for various options and features that can be statically turned
 * on and off
 */

#define debug_list_holder(arg...) arg

#define debug_list_debug_modes                                          \
  debug_list_holder(                                                    \
    active,                "active",                                    \
    barrier,               "barrier",                                   \
    broadcast,             "bcast",                                     \
    event,                 "event",                                     \
    gen,                   "general",                                   \
    group,                 "group",                                     \
    handler,               "handler",                                   \
    hierlb,                "HierarchicalLB",                            \
    lb,                    "lb",                                        \
    location,              "location",                                  \
    param,                 "parameterization",                          \
    pipe,                  "pipe",                                      \
    pool,                  "pool",                                      \
    reduce,                "reduce",                                    \
    rdma,                  "RDMA",                                      \
    rdma_channel,          "RDMA Channel",                              \
    rdma_state,            "RDMA State",                                \
    runtime,               "runtime",                                   \
    scatter,               "scatter",                                   \
    sequence,              "sequencer",                                 \
    sequence_vrt,          "sequencer-vrt",                             \
    serial_msg,            "serialized-msg",                            \
    term,                  "termination",                               \
    termds,                "dijkstra-scholten-TD",                      \
    trace,                 "trace",                                     \
    vrt,                   "vc",                                        \
    vrt_coll,              "vcc",                                       \
    worker,                "worker"                                     \
  )

#define debug_list_debug_options                                        \
  debug_list_holder(                                                    \
    flush,                 "flush all debug prints",                    \
    function,              "print function context_debug",              \
    line_file,             "print line/file debug",                     \
    startup,               "startup sequence"                           \
  )

#define debug_list_features                                             \
  debug_list_holder(                                                    \
    bit_check_overflow,    "Check bitfield overflow",                   \
    detector,              "C++ Trait Detector",                        \
    lblite,                "Load Balancing for Collections",            \
    memory_pool,           "Memory Pooling",                            \
    mpi_rdma,              "Native RDMA with MPI",                      \
    no_feature,            "No feature",                                \
    no_pool_alloc_env,     "No memory pool envelope",                   \
    openmp,                "OpenMP Threading",                          \
    parserdes,             "Partial Inline Serialization",              \
    print_term_msgs,       "Print Termination Control Messages",        \
    production,            "Production Build",                          \
    stdthread,             "std::thread Threading",                     \
    trace_enabled,         "Tracing Projections"                        \
  )

#define debug_list_contexts                                             \
  debug_list_holder(                                                    \
    node,                  "Print current node",                        \
    unknown,               "Print no processor"                         \
  )

#define debug_list_subclass                                             \
  debug_list_holder(                                                    \
    verbose,               "verbose",                                   \
    verbose_2,             "verbose_2"                                  \
  )

#define debug_list_all                          \
  debug_list_holder(                            \
    debug_list_debug_modes,                     \
    debug_list_features,                        \
    debug_list_debug_options,                   \
    debug_list_contexts                         \
  )

#endif  /*INCLUDED_FEATURES_FEATURE_SWITCH*/
