/*
//@HEADER
// ************************************************************************
//
//                          config_assert.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
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

#define argsToString(...) # __VA_ARGS__

#if backend_check_enabled(production)
  #define vtAssert(cond,str,...)
  #define vtAssertInfo(cond,str,...)
  #define vtAssertNot(cond,str,...)
  #define vtAssertNotInfo(cond,str,...)
  #define vtAssertExpr(cond)
#else
  #define vtAssertImpl(fail,cond,str,...)                               \
    do {                                                                \
      if (!(cond)) {                                                    \
        ::vt::debug::assert::assertOut(                                 \
          fail,#cond,str,DEBUG_LOCATION,1, outputArgsImpl(__VA_ARGS__)  \
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
  #define vtAssertArgImpl(fail,cond,str,...)                            \
    do {                                                                \
      if (!(cond)) {                                                    \
        ::vt::debug::assert::assertOutInfo(                             \
          fail,#cond,str,DEBUG_LOCATION,1,                              \
          std::make_tuple(argsToString(__VA_ARGS__)),                   \
          outputArgsImpl(__VA_ARGS__)                                   \
        );                                                              \
      }                                                                 \
    } while (false)
  #define vtAssertExprArgImpl(fail,cond,...)                            \
    vtAssertArgImpl(fail,cond,#cond,__VA_ARGS__)

  #if backend_check_enabled(assert_no_fail)
    #define vtAssert(cond,str,...)     vtAssertImpl(false,cond,str,__VA_ARGS__)
    #define vtAssertInfo(cond,str,...) vtAssertArgImpl(false,cond,str,__VA_ARGS__)
    #define vtAssertExpr(cond)         vtAssertExprImpl(false,cond)
    #define vtAssertExprInfo(cond,...) vtAssertExprArgImpl(false,cond,__VA_ARGS__)
    #define vtWarnInfo(cond,str,...)   vtAssertArgImpl(false,cond,str,__VA_ARGS__)
  #else
    #define vtAssert(cond,str,...)     vtAssertImpl(true,cond,str,__VA_ARGS__)
    #define vtAssertInfo(cond,str,...) vtAssertArgImpl(true,cond,str,__VA_ARGS__)
    #define vtAssertExpr(cond)         vtAssertExprImpl(true,cond)
    #define vtAssertExprInfo(cond,...) vtAssertExprArgImpl(true,cond,__VA_ARGS__)
    #define vtWarnInfo(cond,str,...)   vtAssertArgImpl(false,cond,str,__VA_ARGS__)
  #endif

  #define vtAssertNot(cond,str,...) vtAssert(INVERT_COND(cond),str,__VA_ARGS__)
  #define vtAssertNotExpr(cond)     vtAssertExpr(INVERT_COND(cond))
  #define vtAssertNotInfo(cond,str,...)                              \
    vtAssertInfo(INVERT_COND(cond),str,__VA_ARGS__)
#endif

#endif /*INCLUDED_CONFIGS_ERROR_CONFIG_ASSERT_H*/
