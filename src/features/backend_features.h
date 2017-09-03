
#if ! defined __BACKEND_FEATURES__
#define __BACKEND_FEATURES__

#include "meld.h"
#include "backend_feature_defines.h"

/*
 * Key-value maps for various options and features that can be statically turned
 * on and off
 */

#define debug_list_holder(arg...) arg

#define debug_list_debug_modes                                          \
  debug_list_holder(                                                    \
    gen,                   "general",                                   \
    active,                "active",                                    \
    termination,           "termination",                               \
    event,                 "event",                                     \
    pool,                  "pool",                                      \
    rdma,                  "RDMA",                                      \
    rdma_channel,          "RDMA Channel",                              \
    rdma_state,            "RDMA State",                                \
    param,                 "parameterization",                          \
    handler,               "handler",                                   \
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
    dummy_feature,         "No features yet",                           \
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

#endif
