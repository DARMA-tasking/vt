
#if !defined INCLUDED_DEBUG_MASTER_CONFIG
#define INCLUDED_DEBUG_MASTER_CONFIG

#include <cmake_config.h>

/*
 * Define the compile-time configuration options. Eventually this will be
 * partially defined with cmake options
 */

#define debug_enabled 0
#define debug_force_enabled 0

#include "debug_print.h"

#if !debug_enabled
#define backend_debug_modes backend_options_on(none)
#else
#define backend_debug_modes backend_options_on(                         \
    flush                                                               \
)
#endif

#define backend_features backend_options_on(         \
    detector, default_threading                                 \
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

#endif  /*INCLUDED_DEBUG_MASTER_CONFIG*/
