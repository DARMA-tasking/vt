/*
//@HEADER
// ************************************************************************
//
//                          hard_error.h
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
