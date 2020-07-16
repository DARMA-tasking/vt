/*
//@HEADER
// *****************************************************************************
//
//                               config_assert.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
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

#define PP_NARG(...)  PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)

#define PP_ARG_N(                                  \
    _1, _2, _3, _4, _5, _6, _7, _8, _9,_10,        \
    _11,_12,_13,_14,_15,_16,_17,_18,_19,_20,       \
    _21,_22,_23,_24,_25,_26,_27,_28,_29,_30,       \
    _31,_32,_33,_34,_35,_36,_37,_38,_39,_40,       \
    _41,_42,_43,_44,_45,_46,_47,_48,_49,_50,       \
    _51,_52,_53,_54,_55,_56,_57,_58,_59,_60,       \
    _61,_62,_63, N, ...                            \
  ) N

#define PP_RSEQ_N()                    \
    63,62,61,60,                       \
    59,58,57,56,55,54,53,52,51,50,     \
    49,48,47,46,45,44,43,42,41,40,     \
    39,38,37,36,35,34,33,32,31,30,     \
    29,28,27,26,25,24,23,22,21,20,     \
    19,18,17,16,15,14,13,12,11,10,     \
    9,8,7,6,5,4,3,2,1,0

#define vt_paste_macro(a,b) a ## b
#define vt_xpaste(a,b) vt_paste_macro(a,b)

#define vt_applyx1(X,a)                    X(a)
#define vt_applyx2(X,a,b)                  X(a) X(b)
#define vt_applyx3(X,a,b,c)                X(a) X(b) X(c)
#define vt_applyx4(X,a,b,c,d)              X(a) X(b) X(c) X(d)
#define vt_applyx5(X,a,b,c,d,e)            X(a) X(b) X(c) X(d) X(e)
#define vt_applyx6(X,a,b,c,d,e,f)          X(a) X(b) X(c) X(d) X(e) X(f)
#define vt_applyx7(X,a,b,c,d,e,f,g)        X(a) X(b) X(c) X(d) X(e) X(f) X(g)
#define vt_applyx8(X,a,b,c,d,e,f,g,h)      X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h)
#define vt_applyx9(X,a,b,c,d,e,f,g,h,i)    X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i)
#define vt_applyx10(X,a,b,c,d,e,f,g,h,i,j) X(a) X(b) X(c) X(d) X(e) X(f) X(g) X(h) X(i) X(j)

#define vt_applyx_(M, func, ...) M(func, __VA_ARGS__)
#define vt_applyxn(func,...) vt_applyx_(                     \
    vt_xpaste(vt_applyx, PP_NARG(__VA_ARGS__)), func, __VA_ARGS__  \
  )

#define vt_make_list_strings(a) #a,
#define argsToString(...) vt_applyxn(vt_make_list_strings,__VA_ARGS__)

#if vt_check_enabled(production)
  #define vtAssert(cond,str)            vt_force_use(cond)
  #define vtAssertInfo(cond,str,...)    vt_force_use(cond,__VA_ARGS__)
  #define vtAssertNot(cond,str)         vt_force_use(cond)
  #define vtAssertNotInfo(cond,str,...) vt_force_use(cond,__VA_ARGS__)
  #define vtAssertNotExpr(cond)         vt_force_use(cond)
  #define vtAssertExpr(cond)            vt_force_use(cond)
  #define vtAssertExprInfo(cond,...)    vt_force_use(cond,__VA_ARGS__)
  #define vtWarnInfo(cond,str,...)      vt_force_use(cond,__VA_ARGS__)
/**
 * \internal
 * Assert that the MPI call's return code is MPI_SUCCESS (0).
 *
 * The failure assert contains the MPI call name (eg. "MPI_Iprobe"),
 * short summary (eg. "failed"), and the actual return value.
 */
  #define vtAssertMPISuccess(ret,mpi_name)   vt_force_use(ret)
#else
  #define vtAssertImpl(fail,cond,str)                                   \
    do {                                                                \
      if (!(cond)) {                                                    \
        ::vt::debug::assert::assertOut(                                 \
          fail,#cond,str,DEBUG_LOCATION,1,std::make_tuple()             \
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
          std::make_tuple(argsToString(__VA_ARGS__)""),                 \
          std::make_tuple(__VA_ARGS__,"")                               \
        );                                                              \
      }                                                                 \
    } while (false)
  #define vtAssertExprArgImpl(fail,cond,...)                            \
    vtAssertArgImpl(fail,cond,#cond,__VA_ARGS__)

  #if vt_check_enabled(assert_no_fail)
    #define vtAssert(cond,str)         vtAssertImpl(false,cond,str)
    #define vtAssertInfo(cond,str,...) vtAssertArgImpl(false,cond,str,__VA_ARGS__)
    #define vtAssertExpr(cond)         vtAssertExprImpl(false,cond)
    #define vtAssertExprInfo(cond,...) vtAssertExprArgImpl(false,cond,__VA_ARGS__)
    #define vtWarnInfo(cond,str,...)   vtAssertArgImpl(false,cond,str,__VA_ARGS__)
  #else
    #define vtAssert(cond,str)         vtAssertImpl(true,cond,str)
    #define vtAssertInfo(cond,str,...) vtAssertArgImpl(true,cond,str,__VA_ARGS__)
    #define vtAssertExpr(cond)         vtAssertExprImpl(true,cond)
    #define vtAssertExprInfo(cond,...) vtAssertExprArgImpl(true,cond,__VA_ARGS__)
    #define vtWarnInfo(cond,str,...)   vtAssertArgImpl(false,cond,str,__VA_ARGS__)
  #endif

  #define vtAssertNot(cond,str)        vtAssert(INVERT_COND(cond),str)
  #define vtAssertNotExpr(cond)        vtAssertExpr(INVERT_COND(cond))
  #define vtAssertNotInfo(cond,str,...)                                 \
    vtAssertInfo(INVERT_COND(cond),str,__VA_ARGS__)

  #define vtAssertMPISuccess(ret,mpi_name)  vtAssertInfo(               \
    (ret == 0), "MPI call '" mpi_name "' failed.", ret                  \
  )
#endif

#endif /*INCLUDED_CONFIGS_ERROR_CONFIG_ASSERT_H*/
