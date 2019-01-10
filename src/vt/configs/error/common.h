/*
//@HEADER
// ************************************************************************
//
//                          common.h
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

#if !defined INCLUDED_CONFIGS_ERROR_COMMON_H
#define INCLUDED_CONFIGS_ERROR_COMMON_H

#include "meld_headers.h"
#include "vt/configs/types/types_type.h"

#define outputArgsImpl(args...)                                         \
  meld_if_stmt(                                                         \
    meld_to_bool(_meld_is_empty(args))                                  \
  )()(,args)                                                            \

#define INVERT_COND(cond) (!(cond))
#define DEBUG_LOCATION __FILE__,__LINE__,__func__

#include <string>

namespace vt {

// Forward declare abort and output signatures, defined in collective ops
void abort(std::string const str, ErrorCodeType const code);
void output(std::string const str, ErrorCodeType const code, bool, bool, bool);

namespace debug {

extern NodeType preNode();
extern NodeType preNodes();

}} /* end namespace vt::debug */

#endif /*INCLUDED_CONFIGS_ERROR_COMMON_H*/
