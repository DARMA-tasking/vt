
#if ! defined __DEBUG_CONFIGURATION__
#define __DEBUG_CONFIGURATION__

/*
 * Define the compile-time configuration options. Eventually this will be
 * partially defined with cmake options
 */

#define debug_enabled 0
#define debug_force_enabled 0

#include "debug_print.h"

#if ! debug_enabled
#define backend_debug_modes backend_options_on(none)
#else
#define backend_debug_modes backend_options_on(                         \
    label, flush                                                        \
)
#endif

#define backend_features backend_options_on(    \
    dummy_feature                               \
)

#define backend_debug_contexts backend_options_on(              \
   node, locale, unknown                                        \
)

#define backend_defaults backend_options_on(                    \
    startup                                                     \
)

#endif
