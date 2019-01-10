/*
//@HEADER
// ************************************************************************
//
//                          indexable.impl.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_TYPES_INDEXABLE_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_TYPES_INDEXABLE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/types/type_attorney.h"
#include "vt/vrt/collection/types/migrate_hooks.h"
#include "vt/vrt/collection/types/migratable.h"
#include "vt/vrt/collection/types/indexable.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
Indexable<ColT,IndexT>::Indexable(IndexT&& in_index)
  : Migratable<ColT>(), index_(std::move(in_index)), set_index_(true)
{ }


template <typename ColT, typename IndexT>
IndexT const& Indexable<ColT,IndexT>::getIndex() const {
  if (!set_index_) {
    auto ctx_idx = theCollection()->queryIndexContext<IndexT>();
    vtAssertExpr(ctx_idx != nullptr);
    return *ctx_idx;
  } else {
    return index_;
  }
}

template <typename ColT, typename IndexT>
template <typename SerializerT>
void Indexable<ColT,IndexT>::serialize(SerializerT& s) {
  Migratable<ColT>::serialize(s);
  s | set_index_;
  s | index_;
}

template <typename ColT, typename IndexT>
void Indexable<ColT,IndexT>::setIndex(IndexT const& in_index) {
  // Set the field and then indicate that the `index_` field is now valid with
  // `set_index_`
  index_ = in_index;
  set_index_ = true;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_TYPES_INDEXABLE_IMPL_H*/
