
#if !defined INCLUDED_CONFIGS_ERROR_CONFIG_ASSERT_H
#define INCLUDED_CONFIGS_ERROR_CONFIG_ASSERT_H

/*
 *  Configurable assert allows different behaviors to occur depending on
 *  build/runtime mode when the assertion breaks
 */

#include "configs/debug/debug_config.h"
#include "configs/types/types_type.h"
#include "configs/error/common.h"
#include "configs/error/assert_out.h"
#include "configs/error/assert_out_info.h"
#include "configs/error/keyval_printer.h"

#include <cassert>
#include <tuple>
#include <type_traits>

#define argStringFunc(list_element, ___, not_last)                      \
  meld_if_stmt(not_last)(                                               \
    std::string(#list_element),                                         \
  )(                                                                    \
    std::string(#list_element)                                          \
  )                                                                     \

#define argIdentity(arg) arg

#define argsToString(args...)                                           \
  meld_eval(                                                            \
    meld_map_trans_single(argStringFunc,argIdentity,argIdentity,args)   \
  )

#if backend_check_enabled(production)
  #define vtAssert(cond,str,args...)
  #define vtAssertInfo(cond,str,args...)
  #define vtAssertNot(cond,str,args...)
  #define vtAssertNotInfo(cond,str,args...)
  #define vtAssertExpr(cond)
#else
  #define vtAssertImpl(fail,cond,str,args...)                           \
    do {                                                                \
      if (!(cond)) {                                                    \
        ::vt::debug::assert::assertOut(                                 \
          fail,#cond,str,DEBUG_LOCATION,1 outputArgsImpl(args)          \
        );                                                              \
      }                                                                 \
    } while (false)
  #define vtAssertExprImpl(fail,cond)                                   \
    do {                                                                \
      if (!(cond)) {                                                    \
        ::vt::debug::assert::assertOutExpr(                             \
          fail,#cond,DEBUG_LOCATION,1                                   \
        );                                                              \
      }                                                                 \
    } while (false)
  #define vtAssertArgImpl(fail,cond,str,args...)                        \
    do {                                                                \
      if (!(cond)) {                                                    \
        auto tup = std::make_tuple(argsToString(args));                 \
        ::vt::debug::assert::assertOutInfo(                             \
          fail,#cond,str,DEBUG_LOCATION,1,tup outputArgsImpl(args)      \
        );                                                              \
      }                                                                 \
    } while (false)

  #if backend_check_enabled(assert_no_fail)
    #define vtAssert(cond,str,args...)     vtAssertImpl(false,cond,str,args)
    #define vtAssertInfo(cond,str,args...) vtAssertArgImpl(false,cond,str,args)
    #define vtAssertExpr(cond)             vtAssertExprImpl(false,cond)
  #else
    #define vtAssert(cond,str,args...)     vtAssertImpl(true,cond,str,args)
    #define vtAssertInfo(cond,str,args...) vtAssertArgImpl(true,cond,str,args)
    #define vtAssertExpr(cond)             vtAssertExprImpl(true,cond)
  #endif

  #define vtAssertNot(cond,str,args...)                                 \
    vtAssert(INVERT_COND(cond),str,args)
  #define vtAssertNotInfo(cond,str,args...)                             \
    vtAssertInfo(INVERT_COND(cond),str,args)
#endif

#endif /*INCLUDED_CONFIGS_ERROR_CONFIG_ASSERT_H*/
