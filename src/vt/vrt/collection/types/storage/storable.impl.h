/*
//@HEADER
// *****************************************************************************
//
//                               storable.impl.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_TYPES_STORAGE_STORABLE_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_TYPES_STORAGE_STORABLE_IMPL_H

#include "vt/vrt/collection/types/storage/storable.h"

namespace vt { namespace vrt { namespace collection { namespace storage {

template <typename SerializerT>
void Storable::serialize(SerializerT& s) {
  s | map_;
}

template <typename U>
void Storable::valInsert(std::string const& str, U&& u) {
  map_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(str),
    std::forward_as_tuple(
      std::make_unique<StoreElm<typename std::decay<U>::type>>(
        std::forward<U>(u)
      )
    )
  );
}

template <typename U>
U& Storable::valGet(std::string const& str) {
  vtAssert(valExists(str), "Key must exist in map");
  auto iter = map_.find(str);
  return iter->second->get<U>();
}

template <typename U>
U const& Storable::valGet(std::string const& str) const {
  vtAssert(valExists(str), "Key must exist in map");
  auto iter = map_.find(str);
  return iter->second->get<U>();
}

}}}} /* end namespace vt::vrt::collection::storage */

#endif /*INCLUDED_VT_VRT_COLLECTION_TYPES_STORAGE_STORABLE_IMPL_H*/
