/*
//@HEADER
// ************************************************************************
//
//                          mode.h
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

#if !defined INCLUDED_VT_FETCH_TRAITS_MODE_H
#define INCLUDED_VT_FETCH_TRAITS_MODE_H

#include "vt/config.h"

namespace vt { namespace fetch {

namespace trait {

enum FetchEnum : int8_t {
  Copy = 0x1,
  Ref  = 0x2,
  Read = 0x4
};

} /* end namespace trait */

template <typename std::underlying_type<trait::FetchEnum>::type flag>
struct Traits {
  enum : bool { Read = (flag & trait::FetchEnum::Read) != 0 };
  enum : bool { Copy = (flag & trait::FetchEnum::Copy) != 0 };
  enum : bool { Ref  = (flag & trait::FetchEnum::Ref) != 0  };

  using AddRead = Traits<flag | trait::FetchEnum::Read>;
 };

using Default      = Traits<0x0>;
using Unique       = Traits<trait::FetchEnum::Copy>;
using Ref          = Traits<trait::FetchEnum::Ref>;
using ReadUnique   = Traits<trait::FetchEnum::Read | trait::FetchEnum::Copy>;
using Read         = Traits<trait::FetchEnum::Read | trait::FetchEnum::Ref>;

template <typename T, typename U, typename Enable=void>
struct Down  {
   static constexpr bool is = false;
};

template <typename T, typename U>
struct Down<
  T, U,
  typename std::enable_if_t<
    ((T::Read and U::Read) or (T::Read and not U::Read)) and
    (not T::Copy) and (not U::Copy)
  >
> {
  static constexpr bool is = true;
};

}} /* end namespace vt::fetch */

#endif /*INCLUDED_VT_FETCH_TRAITS_MODE_H*/
