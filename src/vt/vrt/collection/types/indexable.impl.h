/*
//@HEADER
// *****************************************************************************
//
//                               indexable.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

template <typename IndexT>
Indexable<IndexT>::Indexable(IndexT&& in_index)
  : Migratable(),
    index_(std::move(in_index)),
    set_index_(true)
{ }


template <typename IndexT>
IndexT const& Indexable<IndexT>::getIndex() const {
  if (!set_index_) {
    auto ctx_idx = theCollection()->queryIndexContext<IndexT>();
    vtAssertExpr(ctx_idx != nullptr);
    return *ctx_idx;
  } else {
    return index_;
  }
}

template <typename IndexT>
template <typename SerializerT>
void Indexable<IndexT>::serialize(SerializerT& s) {
  Migratable::serialize(s);
  s | set_index_;
  s | index_;
  s | reduce_stamp_;
}

template <typename IndexT>
void Indexable<IndexT>::setIndex(IndexT const& in_index) {
  // Set the field and then indicate that the `index_` field is now valid with
  // `set_index_`
  index_ = in_index;
  set_index_ = true;
}

template <typename IndexT>
void Indexable<IndexT>::zeroReduceStamp() {
  *reduce_stamp_ = 0;
}

template <typename IndexT>
typename Indexable<IndexT>::ReduceStampType Indexable<IndexT>::getNextStamp() {
  ReduceStampType stamp;
  stamp.init<ReduceSeqStampType>(reduce_stamp_);
  ++reduce_stamp_;
  return stamp;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_TYPES_INDEXABLE_IMPL_H*/
