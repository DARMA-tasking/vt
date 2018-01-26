
#if !defined INCLUDED_DEBUG_MASTER_CONFIG
#define INCLUDED_DEBUG_MASTER_CONFIG

/*
 * Define the compile-time configuration options. Eventually this will be
 * partially defined with cmake options
 */

#define debug_enabled 1
#define debug_force_enabled 0

#include "debug_print.h"

#if !debug_enabled
#define backend_debug_modes backend_options_on(none)
#else
#define backend_debug_modes backend_options_on(                         \
    group, flush                                                        \
)
#endif

#define backend_features backend_options_on(         \
    detector, openmp, no_feature.                    \
)

#define backend_debug_contexts backend_options_on(              \
   node, locale, unknown                                        \
)

#define backend_defaults backend_options_on(                    \
   startup                                                      \
)

#define backend_no_threading                                        \
  !backend_check_enabled(openmp) &&                                 \
  !backend_check_enabled(stdthread)

//debug_print(rdma, node, "test");
// backend
// meld_eval_4(backend)
// eval_join(backend,_configuration)

//debug_check_enabled_shortcut(backend, rdma)
//debug_flush_to_out(backend,stdout)

#endif  /*INCLUDED_DEBUG_MASTER_CONFIG*/
