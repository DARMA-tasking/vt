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
void StatsDrivenCollectionMapper<IndexType>::getMap(GetMapMsg* msg) {
  auto node = map(&msg->idx_, msg->idx_.ndims(), theContext()->getNumNodes());
  auto r = msg->request_node_;
  proxy_[r].template send<
    SendMapMsg, &StatsDrivenCollectionMapper<IndexType>::recvMap
  >(msg->idx_, node);
}

template <typename IndexType>
void StatsDrivenCollectionMapper<IndexType>::recvMap(SendMapMsg* msg) {
  rank_mapping_[msg->idx_] = msg->home_node_;
}

template <typename IndexType>
NodeType StatsDrivenCollectionMapper<IndexType>::getOwner(
  const IndexType &idx, int ndim, NodeType num_nodes
) const {
  uint64_t val = 0;
  for (int i = 0; i < ndim; i++) {
    auto dval = static_cast<uint64_t>(idx.get(i));
    val ^= dval << (i * 16);
  }
  auto const owner = static_cast<NodeType>(val % num_nodes);
  return owner;
}

template <typename IndexType>
void StatsDrivenCollectionMapper<IndexType>::notifyOwners(int ndim) {
  runInEpochCollective([=]{
    auto num_nodes = theContext()->getNumNodes();
    auto this_node = theContext()->getNode();
    for (auto item : rank_mapping_) {
      auto owner = getOwner(item.first, ndim, num_nodes);
      if (owner != this_node) {
        proxy_[owner].template send<
          SendMapMsg, &StatsDrivenCollectionMapper<IndexType>::recvMap
        >(item.first, item.second);
      }
    }
  });
}

template <typename IndexType>
NodeType StatsDrivenCollectionMapper<IndexType>::map(
  IndexType* idx, int ndim, NodeType num_nodes
) {
  // if we know, just return what's known
  {
    auto it = rank_mapping_.find(*idx);
    if (it != rank_mapping_.end()) {
      vt_debug_print(
        normal, replay,
        "StatsDrivenCollectionMapper: index {} maps to rank {}\n",
        *idx, it->second
      );
      return it->second;
    }
  }

  // otherwise, phone for help
  auto owner = getOwner(*idx, ndim, num_nodes);
  if (owner == theContext()->getNode()) {
    auto str = fmt::format(
      "map queried about index {}, which was not known to this rank",
      idx->get(0)
    );
    vtAbort(str);
    return uninitialized_destination;
  } else {
    // runInEpochRooted is not DS?
    auto ep = theTerm()->makeEpochRooted("mapTest", term::UseDS{true});
    theMsg()->pushEpoch(ep);
    proxy_[owner].template send<
      GetMapMsg, &StatsDrivenCollectionMapper<IndexType>::getMap
    >(*idx, theContext()->getNode());
    theMsg()->popEpoch(ep);
    theTerm()->finishedEpoch(ep);
    runSchedulerThrough(ep);
    auto it = rank_mapping_.find(*idx);
    vtAssert(it != rank_mapping_.end(), "Home rank still not known");
    return it->second;
  }
}

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
