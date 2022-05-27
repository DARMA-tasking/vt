/*
//@HEADER
// *****************************************************************************
//
//                          manager.collection.impl.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_MANAGER_COLLECTION_IMPL_H
#define INCLUDED_VT_RDMAHANDLE_MANAGER_COLLECTION_IMPL_H

#include "vt/config.h"
#include "vt/rdmahandle/manager.h"
#include "vt/rdmahandle/sub_handle.h"
#include "vt/vrt/collection/manager.h"
#include "vt/phase/phase_manager.h"

namespace vt { namespace rdma {

template <typename T, HandleEnum E, typename ProxyT, typename ColT>
void Manager::informCollectionRDMA(
  impl::InformRDMAMsg<ProxyT, typename ColT::IndexType>* msg
) {
  auto collection_proxy = msg->proxy_;
  auto next_handle = msg->rdma_handle_;
  auto uniform = msg->uniform_size_;
  auto map = msg->map_han_;
  auto range = msg->range_;
  makeCollectionHandles<T,E,ColT,ProxyT>(
    collection_proxy, 0, uniform, next_handle, map, range
  );
}

template <
  typename T,
  HandleEnum E,
  typename ColT,
  typename ProxyT,
  typename IndexT
>
Handle<T, E, IndexT> Manager::makeCollectionHandles(
  ProxyT collection_proxy, std::size_t idx_size, bool uniform_size,
  RDMA_HandleType in_next_handle, vt::HandlerType map_han, IndexT in_range
) {
  using ElementListener = vrt::collection::listener::ElementEventEnum;
  using SubType         = SubHandle<T, vt::rdma::HandleEnum::StaticSize, IndexT>;
  using ListenerType    = vrt::collection::listener::ListenFnType<IndexT>;

  RDMA_HandleType next_handle = in_next_handle;
  auto proxy_bits = collection_proxy.getCollectionProxy();
  auto idx = collection_proxy.getElementProxy().getIndex();
  IndexT range = in_range;
  if (in_next_handle == no_rdma_handle) {
    range = theCollection()->getRange<ColT>(proxy_bits);
    auto lin = vt::mapping::linearizeDenseIndexRowMajor(&idx, &range);
    next_handle = ++cur_handle_collection_[proxy_bits][lin];
  }

  vt_debug_print(
    normal, rdma,
    "CollectionHandle: next_handle={}, idx={}, range={}\n",
    next_handle, idx, range
  );

  objgroup::proxy::Proxy<SubType> sub_proxy;
  auto iter = collection_to_manager_[proxy_bits].find(next_handle);
  if (iter == collection_to_manager_[proxy_bits].end()) {
    // First time this handle is being created on this node
    if (map_han == -1) {
      map_han = theCollection()->getTypelessHolder().getMap(proxy_bits);
    }
    sub_proxy = SubType::construct(true, range, true, map_han);
    // Register the migration listener
    ListenerType fn = [=](ElementListener event, IndexT lidx, NodeType) {
      if (event == ElementListener::ElementMigratedOut) {
        sub_proxy.get()->migratedOutIndex(lidx);
      } else if (event == ElementListener::ElementMigratedIn) {
        sub_proxy.get()->migratedInIndex(lidx);
      }
    };
    theCollection()->template registerElementListener<ColT>(proxy_bits, fn);

    // Count the number of local handles that should exist here. We can't use the
    // cached value in the collection manager since this might be invoked in the
    // an element's constructor before they are all created.
    auto this_node = theContext()->getNode();
    auto num_nodes = theContext()->getNumNodes();
    auto min_node_mapped = theContext()->getNumNodes();
    std::size_t local_count = 0;
    auto const& map_fn = auto_registry::getHandlerMap(map_han);
    range.foreach ([&](IndexT cur_idx) {
      auto home_node = map_fn->dispatch(&cur_idx, &range, num_nodes);
      if (home_node == this_node) {
        local_count++;
      }
      min_node_mapped = std::min(home_node, min_node_mapped);
    });

    // If LB is enabled then we need to register an afterLB listener
#   if vt_check_enabled(lblite)
    vt_debug_print(
      verbose, rdma,
      "CollectionHandle: registering LB listener\n"
    );
    thePhase()->registerHookCollective(phase::PhaseHook::EndPostMigration, [=]{
      sub_proxy.get()->afterLB();
    });
#   endif

    // Set the number of expected handles for next time we run through this code
    sub_proxy.get()->setCollectionExpected(local_count);

    if (local_count == 0) {
      sub_proxy.get()->makeSubHandles();
    }

    collection_to_manager_[proxy_bits][next_handle] = sub_proxy.getProxy();

    // If this node is the min node mapped, it is responsible for informing all
    // other nodes about the RDMA handle creation, since it uses a collective
    // ObjGroup and all nodes might not have a collection element mapped to
    // them.
    if (this_node == min_node_mapped and in_next_handle == no_rdma_handle) {
      proxy_.template broadcast<
        impl::InformRDMAMsg<ProxyT,IndexT>,
        &Manager::informCollectionRDMA<T,E,ProxyT,ColT>
      >(collection_proxy, next_handle, uniform_size, map_han, range);
    }
  } else {
    auto sub_proxy_bits = iter->second;
    sub_proxy = objgroup::proxy::Proxy<SubType>(sub_proxy_bits);
  }

  if (in_next_handle == no_rdma_handle) {
    // Add the handle
    auto handle = sub_proxy.get()->addLocalIndex(idx, idx_size);

    // Check if all mapped handles are now created on this node
    auto expected = sub_proxy.get()->getCollectionExpected();
    auto current = sub_proxy.get()->getNumHandles();

    vt_debug_print(
      verbose, rdma,
      "CollectionHandle: expected={}, current={}\n", expected, current
    );

    // All the local handles have been added
    if (current == expected) {
      sub_proxy.get()->makeSubHandles();
    }

    return handle;
  } else {
    return Handle<T, E, IndexT>();
  }
}

}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_MANAGER_COLLECTION_IMPL_H*/
