
#if !defined INCLUDED_FEATURES_FEATURE_SWITCH
#define INCLUDED_FEATURES_FEATURE_SWITCH

#include "meld/meld_headers.h"
#include "features_defines.h"

/*
 * Key-value maps for various options and features that can be statically turned
 * on and off
 */

#define debug_list_holder(arg...) arg

#define debug_list_debug_modes                                          \
  debug_list_holder(                                                    \
    gen,                   "general",                                   \
    active,                "active",                                    \
    term,                  "termination",                               \
    barrier,               "barrier",                                   \
    event,                 "event",                                     \
    pool,                  "pool",                                      \
    rdma,                  "RDMA",                                      \
    rdma_channel,          "RDMA Channel",                              \
    rdma_state,            "RDMA State",                                \
    param,                 "parameterization",                          \
    handler,               "handler",                                   \
    sequence,              "sequencer",                                 \
    sequence_vrt,          "sequencer-vrt",                             \
    trace,                 "trace",                                     \
    vrt,                   "virtual-context",                           \
    worker,                "worker",                                    \
    location,              "location"                                   \
  )

#define debug_list_debug_options                                        \
  debug_list_holder(                                                    \
    startup,               "startup sequence",                          \
    flush,                 "flush all debug prints",                    \
    line_file,             "print line/file debug",                     \
    function,              "print function context_debug"               \
  )

#define debug_list_features                                             \
  debug_list_holder(                                                    \
    no_feature,            "No feature",                                \
    bit_check_overflow,    "Check bitfield overflow",                   \
    trace_enabled,         "Trace",                                     \
    detector,              "Detector"                                   \
  )

#define debug_list_contexts                                             \
  debug_list_holder(                                                    \
    node,                  "Print current node",                        \
    unknown,               "Print no processor"                         \
  )

#define debug_list_all                          \
  debug_list_holder(                            \
    debug_list_debug_modes,                     \
    debug_list_features,                        \
    debug_list_debug_options,                   \
    debug_list_contexts                         \
  )

#endif  /*INCLUDED_FEATURES_FEATURE_SWITCH*/
