
#if !defined INCLUDED_FEATURES_FEATURE_SWITCH
#define INCLUDED_FEATURES_FEATURE_SWITCH

#include "meld_headers.h"
#include "features_defines.h"

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
    lb,                    "lb",                                        \
    location,              "location",                                  \
    param,                 "parameterization",                          \
    pool,                  "pool",                                      \
    rdma,                  "RDMA",                                      \
    rdma_channel,          "RDMA Channel",                              \
    rdma_state,            "RDMA State",                                \
    runtime,               "runtime",                                   \
    sequence,              "sequencer",                                 \
    sequence_vrt,          "sequencer-vrt",                             \
    serial_msg,            "serialized-msg",                            \
    term,                  "termination",                               \
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
    memory_pool,           "Enable memory pooling",                     \
    no_feature,            "No feature",                                \
    no_pool_alloc_env,     "Disable memory pool envelope",              \
    openmp,                "OpenMP Threading",                          \
    print_term_msgs,       "Debug-prints-TD-msgs",                      \
    runtime_checks,        "Runtime Fidelity Checking",                 \
    stdthread,             "std::thread Threading",                     \
    trace_enabled,         "Trace"                                      \
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
