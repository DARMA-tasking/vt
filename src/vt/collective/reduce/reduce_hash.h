/*
//@HEADER
// ************************************************************************
//
//                          reduce_hash.h
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

#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_HASH_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_HASH_H

#include "vt/config.h"

#include <tuple>

namespace vt { namespace collective { namespace reduce {

using ReduceIdentifierType =
  std::tuple<TagType,SequentialIDType,VirtualProxyType,ObjGroupProxyType>;
using ReduceSeqLookupType =
  std::tuple<VirtualProxyType,TagType,ObjGroupProxyType>;

}}} /* end namespace vt::collective::reduce */

namespace std {

using ReduceIDType = ::vt::collective::reduce::ReduceIdentifierType;
using ReduceLookupType = ::vt::collective::reduce::ReduceSeqLookupType;

template <>
struct hash<ReduceIDType> {
  size_t operator()(ReduceIDType const& in) const {
    auto const& combined =
      std::hash<vt::TagType>()(std::get<0>(in)) ^
      std::hash<vt::SequentialIDType>()(std::get<1>(in)) ^
      std::hash<vt::VirtualProxyType>()(std::get<2>(in)) ^
      std::hash<vt::ObjGroupProxyType>()(std::get<3>(in));
    return combined;
  }
};

template <>
struct hash<ReduceLookupType> {
  size_t operator()(ReduceLookupType const& in) const {
    auto const& combined =
      std::hash<vt::VirtualProxyType>()(std::get<0>(in)) ^
      std::hash<vt::TagType>()(std::get<1>(in)) ^
      std::hash<vt::ObjGroupProxyType>()(std::get<2>(in));
    return combined;
  }
};

} /* end namespace std */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_HASH_H*/
