/*
//@HEADER
// ************************************************************************
//
//                          config_assert.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
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

#if !defined INCLUDED_CONFIGS_ERROR_CONFIG_ASSERT_H
#define INCLUDED_CONFIGS_ERROR_CONFIG_ASSERT_H

/*
 *  Configurable assert allows different behaviors to occur depending on
 *  build/runtime mode when the assertion breaks
 */

#include "vt/configs/debug/debug_config.h"
#include "vt/configs/types/types_type.h"
#include "vt/configs/error/common.h"
#include "vt/configs/error/assert_out.h"
#include "vt/configs/error/assert_out_info.h"
#include "vt/configs/error/keyval_printer.h"

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
  #define vtAssertExprArgImpl(fail,cond,args...)                        \
    vtAssertArgImpl(fail,cond,#cond,args)

  #if backend_check_enabled(assert_no_fail)
    #define vtAssert(cond,str,args...)     vtAssertImpl(false,cond,str,args)
    #define vtAssertInfo(cond,str,args...) vtAssertArgImpl(false,cond,str,args)
    #define vtAssertExpr(cond)             vtAssertExprImpl(false,cond)
    #define vtAssertExprInfo(cond,args...) vtAssertExprArgImpl(false,cond,args)
    #define vtWarnInfo(cond,str,args...)   vtAssertArgImpl(false,cond,str,args)
  #else
    #define vtAssert(cond,str,args...)     vtAssertImpl(true,cond,str,args)
    #define vtAssertInfo(cond,str,args...) vtAssertArgImpl(true,cond,str,args)
    #define vtAssertExpr(cond)             vtAssertExprImpl(true,cond)
    #define vtAssertExprInfo(cond,args...) vtAssertExprArgImpl(true,cond,args)
    #define vtWarnInfo(cond,str,args...)   vtAssertArgImpl(false,cond,str,args)
  #endif

  #define vtAssertNot(cond,str,args...) vtAssert(INVERT_COND(cond),str,args)
  #define vtAssertNotExpr(cond)         vtAssertExpr(INVERT_COND(cond))
  #define vtAssertNotInfo(cond,str,args...)                              \
    vtAssertInfo(INVERT_COND(cond),str,args)
#endif

#endif /*INCLUDED_CONFIGS_ERROR_CONFIG_ASSERT_H*/
