/*
//@HEADER
// *****************************************************************************
//
//                         collection_chain_set.impl.h
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

#if !defined INCLUDED_VT_MESSAGING_COLLECTION_CHAIN_SET_IMPL_H
#define INCLUDED_VT_MESSAGING_COLLECTION_CHAIN_SET_IMPL_H

#include "vt/messaging/collection_chain_set.h"
#include "vt/vrt/collection/listener/listen_events.h"

namespace vt { namespace messaging {

template <typename Index>
template <typename ProxyT, typename IndexT>
CollectionChainSet<Index>::CollectionChainSet(
  ProxyT proxy, ChainSetLayout layout
) {
  using ColT = typename ProxyT::CollectionType;
  using ThisType = CollectionChainSet<Index>;
  using ListenerType = vrt::collection::listener::ListenFnType<IndexT>;
  using vrt::collection::listener::ElementEventEnum;

  static_assert(std::is_same<IndexT, Index>::value, "Must match index type");

  // Make this chain set an objgroup instance so we can send updates
  auto p = theObjGroup()->makeCollective<CollectionChainSet<Index>>(this);

  auto const this_node = theContext()->getNode();
  auto const proxy_bits = proxy.getProxy();

  ListenerType l = [=](ElementEventEnum event, IndexT idx, NodeType home) {
    switch (event) {
    case ElementEventEnum::ElementCreated:
    case ElementEventEnum::ElementMigratedIn:
      if (layout == Local or (layout == Home and home == this_node)) {
        if (chains_.find(idx) == chains_.end()) {
          addIndex(idx);
        } else {
          vtAssert(
            layout == Home and event == ElementEventEnum::ElementMigratedIn,
            "Must be home layout and migrated in"
          );
        }
      } else if (event != ElementEventEnum::ElementMigratedIn) {
        vtAssert(layout == Home, "Must be a home layout");
        p[home].template send<IdxMsg, &ThisType::addIndexHan>(idx);
      }
      break;
    case ElementEventEnum::ElementDestroyed:
      if (chains_.find(idx) != chains_.end()) {
        removeIndex(idx);
      } else {
        vtAssert(layout == Home, "Must be a home layout");
        p[home].template send<IdxMsg, &ThisType::removeIndexHan>(idx);
      }
    case ElementEventEnum::ElementMigratedOut:
      if (layout == Local) {
        removeIndex(idx);
      }
      break;
    }
  };

  // Register the listener with the system
  auto listener = theCollection()->template registerElementListener<ColT>(
    proxy_bits, l
  );

  // Any elements that currently exist must be added as they won't be triggered
  // by the listener
  auto const& local_set = theCollection()->getLocalIndices(proxy);
  for (auto&& idx : local_set) {
    auto const home = theCollection()->getMappedNode(proxy, idx);
    if (layout == Local or (layout == Home and home == this_node)) {
      addIndex(idx);
    } else {
      p[home].template send<IdxMsg, &ThisType::addIndexHan>(idx);
    }
  }

  deallocator_ = [=]{
    theCollection()->template unregisterElementListener<ColT>(
      proxy_bits, listener
    );
  };
}

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_COLLECTION_CHAIN_SET_IMPL_H*/
