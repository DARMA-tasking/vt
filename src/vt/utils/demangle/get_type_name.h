/*
//@HEADER
// ************************************************************************
//
//                          get_type_name.h
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

#if !defined INCLUDED_UTILS_DEMANGLE_GET_TYPE_NAME_H
#define INCLUDED_UTILS_DEMANGLE_GET_TYPE_NAME_H

#include "vt/config.h"
#include "vt/utils/string/static.h"

namespace vt { namespace util { namespace demangle {

using StaticString = util::string::StatStr;

template <class T>
constexpr StaticString type_name() {
#if defined(__clang__)
  StaticString p = __PRETTY_FUNCTION__;
  return StaticString(p.data() + 31, p.size() - 31 - 1);

# elif defined(__GNUC__)
  StaticString p = __PRETTY_FUNCTION__;

#   if __cplusplus < 201402
    return StaticString(p.data() + 36, p.size() - 36 - 1);
#   else
    return StaticString(p.data() + 46, p.size() - 46 - 1);
#   endif

# elif defined(_MSC_VER)
  StaticString p = __FUNCSIG__;
  return StaticString(p.data() + 38, p.size() - 38 - 7);
#endif
}

}}} /* end namespace vt::util::demangle */

#endif /*INCLUDED_UTILS_DEMANGLE_GET_TYPE_NAME_H*/
