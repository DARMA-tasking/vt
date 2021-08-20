/*
//@HEADER
// *****************************************************************************
//
//                   stats_driven_collection_mapper.impl.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_MAPPER_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_MAPPER_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/stats_driven_collection_mapper.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

template <typename IndexType>
void StatsDrivenCollectionMapper<IndexType>::addCollectionMapping(
  IndexType idx, NodeType home
) {
  rank_mapping_[idx] = home;
}

template <typename IndexType>
void StatsDrivenCollectionMapper<IndexType>::addElmToIndexMapping(
  ElmIDType elm_id, IndexType index
) {
  elm_to_index_mapping_[elm_id] = index;
}

template <typename IndexType>
void StatsDrivenCollectionMapper<IndexType>::addElmToIndexMapping(
  ElmIDType elm_id, IndexVec idx_vec
) {
  IndexType index;
  int i=0;
  for (auto &entry : idx_vec) {
    index[i++] = entry;
  }
  elm_to_index_mapping_[elm_id] = index;
}

template <typename IndexType>
IndexType StatsDrivenCollectionMapper<IndexType>::getIndexFromElm(ElmIDType elm_id) {
  auto idxiter = elm_to_index_mapping_.find(elm_id);
  vtAssert(
    idxiter != elm_to_index_mapping_.end(),
    "Element ID to index mapping must be known"
  );
  auto index = idxiter->second;
  return index;
}

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_MAPPER_IMPL_H*/
