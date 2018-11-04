
#if !defined INCLUDED_CONFIGS_ERROR_HARD_ERROR_H
#define INCLUDED_CONFIGS_ERROR_HARD_ERROR_H

/*
 *  A hard error is always checked and leads to failure in any mode if
 *  triggered
 */

#include "vt/configs/debug/debug_config.h"
#include "vt/configs/types/types_type.h"
#include "vt/configs/error/common.h"
#include "vt/configs/error/error.h"

#include <string>
#include <tuple>
#include <type_traits>

#if backend_check_enabled(production)
  #define vtAbort(str,args...)                                            \
    ::vt::error::display(str,1  outputArgsImpl(args));
  #define vtAbortCode(xy,str,args...)                                     \
    ::vt::error::display(str,xy outputArgsImpl(args));
#else
  #define vtAbort(str,args...)                                            \
    ::vt::error::displayLoc(str,1, DEBUG_LOCATION outputArgsImpl(args));
  #define vtAbortCode(xy,str,args...)                                     \
    ::vt::error::displayLoc(str,xy,DEBUG_LOCATION outputArgsImpl(args));
#endif

#define vtAbortIf(cond,str,args...)                                       \
  do {                                                                    \
    if ((cond)) {                                                         \
      vtAbort(str,args);                                                  \
    }                                                                     \
  } while (false)
#define vtAbortIfCode(code,cond,str,args...)                              \
  do {                                                                    \
    if ((cond)) {                                                         \
      vtAbortCode(code,str,args);                                         \
    }                                                                     \
  } while (false)

#define vtAbortIfNot(cond,str,args...)                                    \
  vtAbortIf(INVERT_COND(cond),str,args)
#define vtAbortIfNotCode(code,cond,str,args...)                           \
  vtAbortIfCode(code,INVERT_COND(cond),str,args)

#endif /*INCLUDED_CONFIGS_ERROR_HARD_ERROR_H*/
