
#if !defined INCLUDED_CONFIGS_ERROR_CONFIG_ASSERT_H
#define INCLUDED_CONFIGS_ERROR_CONFIG_ASSERT_H

/*
 *  Configurable assert allows different behaviors to occur depending on
 *  build/runtime mode when the assertion breaks
 */

#include "configs/debug/debug_config.h"

#if backend_check_enabled(production)
  #define vtAssert(cond,str)
#else
  #if backend_check_enabled(assert_no_fail)
    #define vtAssert(cond,str)                      \
      do {                                          \
        if (!(cond)) {                              \
          ::vt::output(str);                        \
        }                                           \
      } while (false)
  #else
    #define vtAssert(cond,str)                      \
      do {                                          \
        if (!(cond)) {                              \
          assert((cond) && (str));                  \
        }                                           \
      } while (false)
  #endif
#endif

#endif /*INCLUDED_CONFIGS_ERROR_CONFIG_ASSERT_H*/
