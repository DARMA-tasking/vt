/*
//@HEADER
// *****************************************************************************
//
//                                manager.impl.h
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

#if !defined INCLUDED_VRT_COLLECTION_MANAGER_IMPL_H
#define INCLUDED_VRT_COLLECTION_MANAGER_IMPL_H

#include "vt/config.h"
#include "vt/topos/location/location_headers.h"
#include "vt/context/context.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vt/vrt/collection/manager.h"
#include "vt/vrt/collection/messages/system_create.h"
#include "vt/vrt/collection/collection_info.h"
#include "vt/vrt/collection/messages/user.h"
#include "vt/vrt/collection/messages/user_wrap.h"
#include "vt/vrt/collection/types/type_attorney.h"
#include "vt/vrt/collection/defaults/default_map.h"
#include "vt/vrt/collection/constructor/coll_constructors_deref.h"
#include "vt/vrt/collection/migrate/migrate_msg.h"
#include "vt/vrt/collection/migrate/migrate_handlers.h"
#include "vt/vrt/collection/active/active_funcs.h"
#include "vt/vrt/collection/destroy/destroy_msg.h"
#include "vt/vrt/collection/destroy/destroy_handlers.h"
#include "vt/vrt/collection/balance/phase_msg.h"
#include "vt/vrt/collection/balance/lb_listener.h"
#include "vt/vrt/collection/dispatch/dispatch.h"
#include "vt/vrt/collection/dispatch/registry.h"
#include "vt/vrt/collection/holders/insert_context_holder.h"
#include "vt/vrt/collection/collection_directory.h"
#include "vt/vrt/collection/balance/node_stats.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/registry/auto/map/auto_registry_map.h"
#include "vt/registry/auto/collection/auto_registry_collection.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/topos/mapping/mapping_headers.h"
#include "vt/termination/term_headers.h"
#include "vt/serialization/sizer.h"
#include "vt/collective/reduce/reduce_hash.h"
#include "vt/runnable/collection.h"
#include "vt/group/group_headers.h"
#include "vt/pipe/pipe_headers.h"
#include "vt/scheduler/scheduler.h"

#include <tuple>
#include <utility>
#include <functional>
#include <cassert>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

#include "fmt/format.h"
#include "fmt/ostream.h"

namespace vt { namespace vrt { namespace collection {

namespace details {
template <typename T>
/*static*/ VirtualIDType CurIdent<T>::m_ = 0;

template <typename ColT>
/*static*/ CollectionManager::BcastBufferType<ColT>
Broadcasts<ColT>::m_ = {};
}

template <typename>
void CollectionManager::cleanupAll() {
  /*
   *  Run the cleanup functions for type-specific cleanup that can not be
   *  performed without capturing the type of each collection
   */
  int num_cleanup = 0;
  int tot_cleanup = static_cast<int>(cleanup_fns_.size());
  auto iter = cleanup_fns_.begin();
  while (iter != cleanup_fns_.end()) {
    auto lst = std::move(iter->second);
    iter = cleanup_fns_.erase(iter);
    num_cleanup++;
    for (auto fn : lst) {
      fn();
    }
  }
  vtAssertExpr(cleanup_fns_.size() == 0);
  vtAssertExpr(num_cleanup == tot_cleanup);
}

template <typename>
void CollectionManager::destroyCollections() {
  UniversalIndexHolder<>::destroyAllLive();
}

template <typename ColT, typename IndexT, typename Tuple, size_t... I>
/*static*/ typename CollectionManager::VirtualPtrType<ColT, IndexT>
CollectionManager::runConstructor(
  VirtualElmCountType const& elms, IndexT const& idx, Tuple* tup,
  std::index_sequence<I...>
) {
  return std::make_unique<ColT>(
    std::forward<typename std::tuple_element<I,Tuple>::type>(
      std::get<I>(*tup)
    )...
  );
}

template <typename ColT>
void CollectionManager::addCleanupFn(VirtualProxyType proxy) {
  cleanup_fns_[proxy].push_back([=]{
    CollectionProxyWrapType<ColT> typed_proxy(proxy);
    destroyMatching(typed_proxy);
  });
}

template <typename SysMsgT>
/*static*/ void CollectionManager::distConstruct(SysMsgT* msg) {
  using ColT        = typename SysMsgT::CollectionType;
  using IndexT      = typename SysMsgT::IndexType;
  using BaseIdxType = vt::index::BaseIndex;

  auto const num_nodes = theContext()->getNumNodes();

  auto& info = msg->info;

  // The VirtualProxyType for this construction
  auto proxy = info.getProxy();
  // The insert epoch for this collection
  auto const insert_epoch = info.getInsertEpoch();
  // Count the number of elements locally created here
  int64_t num_elements_created = 0;
  // Get the mapping function handle
  auto const map_han = msg->map;
  // Get the range for the construction
  auto range = msg->info.range_;

  theCollection()->insertCollectionInfo(proxy,msg->map,insert_epoch);

  // Add the cleanup function for this node
  theCollection()->addCleanupFn<ColT>(proxy);

  if (info.immediate_) {
    // Get the handler function
    auto fn = auto_registry::getHandlerMap(map_han);
    // Total count across the statically sized collection
    std::size_t num_elms = info.range_.getSize();

    vt_debug_print(
      vrt_coll, node,
      "running foreach: size={}, range={}, map_han={}\n",
      num_elms, range, map_han
    );

    range.foreach([&](IndexT cur_idx) mutable {
      vt_debug_print_verbose(
        vrt_coll, node,
        "running foreach: before map: cur_idx={}, range={}\n",
        cur_idx.toString(), range.toString()
      );

      auto const cur = static_cast<BaseIdxType*>(&cur_idx);
      auto const max = static_cast<BaseIdxType*>(&range);

      auto mapped_node = fn(cur, max, num_nodes);

      vt_debug_print(
        vrt_coll, node,
        "construct: foreach: node={}, cur_idx={}, max_range={}\n",
        mapped_node, cur_idx.toString(), range.toString()
      );

      if (theContext()->getNode() == mapped_node) {
        using IdxContextHolder = InsertContextHolder<IndexT>;

        // Actually construct the element. If the detector is enabled, call the
        // detection-based overloads to invoke the constructor with the
        // optionally-positional index. Otherwise, invoke constructor without
        // the index as a parameter

        // Set the current context index to `cur_idx`. This enables the user to
        // query the index of their collection element in the constructor, which
        // is often very handy
        IdxContextHolder::set(&cur_idx,proxy);

        #if vt_check_enabled(detector) && vt_check_enabled(cons_multi_idx)
          auto new_vc = DerefCons::derefTuple<ColT, IndexT, decltype(msg->tup)>(
            num_elms, cur_idx, &msg->tup
          );
        #else
          using Args = typename SysMsgT::ArgsTupleType;
          static constexpr auto num_args = std::tuple_size<Args>::value;
          auto new_vc = CollectionManager::runConstructor<ColT, IndexT>(
            num_elms, cur_idx, &msg->tup, std::make_index_sequence<num_args>{}
          );
        #endif

        /*
         * Set direct attributes of the newly constructed element directly on
         * the user's class
         */
        CollectionTypeAttorney::setup(new_vc, num_elms, cur_idx, proxy);

        // Insert the element into the managed holder for elements
        theCollection()->insertCollectionElement<ColT, IndexT>(
          std::move(new_vc), cur_idx, msg->info.range_, map_han, proxy,
          info.immediate_, mapped_node
        );

        // Clear the current index context
        IdxContextHolder::clear();

        // Increment the number of elements created locally
        num_elements_created++;
      }
    });
  } else {
    vtAssert(num_elements_created == 0, "Elements should not be created");
  }

  if (num_elements_created == 0) {
    insertMetaCollection<ColT>(proxy,map_han,range,info.immediate_);
  }

  // Reduce construction of the collection to release dependencies when
  // construction has finsihed
  reduceConstruction<ColT>(proxy);

  // Construct a underlying group for the collection
  groupConstruction<ColT>(proxy,info.immediate_);
}

template <typename ColT, typename IndexT>
std::size_t CollectionManager::groupElementCount(VirtualProxyType const& p) {
  auto elm_holder = theCollection()->findElmHolder<ColT,IndexT>(p);
  std::size_t const num_elms = elm_holder->numElements();

  vt_debug_print(
    vrt_coll, node,
    "groupElementcount: num_elms={}, proxy={:x}, proxy={:x}\n",
    num_elms, p, p
  );

  return num_elms;
}

template <typename ColT, typename IndexT>
GroupType CollectionManager::createGroupCollection(
  VirtualProxyType const& proxy, bool const in_group
) {
  vt_debug_print(
    vrt_coll, node,
    "createGroupCollection: proxy={:x}, in_group={}\n",
    proxy, in_group
  );

  auto const group_id = theGroup()->newGroupCollective(
    in_group, [proxy](GroupType new_group){
      auto const& group_root = theGroup()->groupRoot(new_group);
      auto const& is_group_default = theGroup()->groupDefault(new_group);
      auto const& my_in_group = theGroup()->inGroup(new_group);
      auto elm_holder = theCollection()->findElmHolder<ColT,IndexT>(proxy);
      elm_holder->setGroup(new_group);
      elm_holder->setUseGroup(!is_group_default);
      elm_holder->setGroupReady(true);
      if (!is_group_default) {
        elm_holder->setGroupRoot(group_root);
      }

      vt_debug_print(
        vrt_coll, node,
        "group finished construction: proxy={:x}, new_group={:x}, use_group={}, "
        "ready={}, root={}, is_group_default={}\n",
        proxy, new_group, elm_holder->useGroup(), elm_holder->groupReady(),
        group_root, is_group_default
      );

      if (!is_group_default && my_in_group) {
        using collective::reduce::makeStamp;
        using collective::reduce::StrongUserID;

        auto group_msg = makeMessage<CollectionGroupMsg>(proxy,new_group);
        vt_debug_print(
          vrt_coll, node,
          "calling group (construct) reduce: proxy={:x}\n", proxy
        );
        auto r = theGroup()->groupReducer(new_group);

        auto stamp = makeStamp<StrongUserID>(proxy);
        r->reduce<CollectionGroupMsg, collectionGroupReduceHan>(
          group_root, group_msg.get(), stamp
        );
      } else if (is_group_default) {
        /*
         *  Trigger the group finished handler directly because the default
         *  group will now be utilized
         */
        auto nmsg = makeMessage<CollectionGroupMsg>(proxy,new_group);
        theCollection()->collectionGroupFinishedHan<>(nmsg.get());
      }
    }
  );

  vt_debug_print(
    vrt_coll, node,
    "createGroupCollection (after): proxy={:x}, in_group={}, group_id={:x}\n",
    proxy, in_group, group_id
  );

  return group_id;
}

template <typename ColT, typename IndexT, typename MsgT, typename UserMsgT>
/*static*/ CollectionManager::IsWrapType<ColT,UserMsgT,MsgT>
CollectionManager::collectionAutoMsgDeliver(
  MsgT* msg, CollectionBase<ColT,IndexT>* base, HandlerType han, bool member,
  NodeType from, trace::TraceEventIDType event
) {
  using IdxContextHolder = InsertContextHolder<IndexT>;

  auto& user_msg = msg->getMsg();
  auto user_msg_ptr = &user_msg;
  // Be careful with type casting here..convert to typeless before
  // reinterpreting the pointer so the compiler does not produce the wrong
  // offset
  void* raw_ptr = static_cast<void*>(base);
  auto ptr = reinterpret_cast<UntypedCollection*>(raw_ptr);

  // Expand out the index for tracing purposes; Projections takes up to
  // 4-dimensions
  auto idx = base->getIndex();
  uint64_t const idx1 = idx.ndims() > 0 ? idx[0] : 0;
  uint64_t const idx2 = idx.ndims() > 1 ? idx[1] : 0;
  uint64_t const idx3 = idx.ndims() > 2 ? idx[2] : 0;
  uint64_t const idx4 = idx.ndims() > 3 ? idx[3] : 0;

  // Set the current context index, enabling the user to query the index of
  // their collection element
  auto const proxy = base->getProxy();
  IdxContextHolder::set(&idx,proxy);

  runnable::RunnableCollection<UserMsgT,UntypedCollection>::run(
    han, user_msg_ptr, ptr, from, member, idx1, idx2, idx3, idx4
  );

  // Clear the current index context
  IdxContextHolder::clear();
}

template <typename ColT, typename IndexT, typename MsgT, typename UserMsgT>
/*static*/ CollectionManager::IsNotWrapType<ColT,UserMsgT,MsgT>
CollectionManager::collectionAutoMsgDeliver(
  MsgT* msg, CollectionBase<ColT,IndexT>* base, HandlerType han, bool member,
  NodeType from, trace::TraceEventIDType event
) {
  using IdxContextHolder = InsertContextHolder<IndexT>;

  // Be careful with type casting here..convert to typeless before
  // reinterpreting the pointer so the compiler does not produce the wrong
  // offset
  void* raw_ptr = static_cast<void*>(base);
  auto ptr = reinterpret_cast<UntypedCollection*>(raw_ptr);

  // Expand out the index for tracing purposes; Projections takes up to
  // 4-dimensions
  auto idx = base->getIndex();
  uint64_t const idx1 = idx.ndims() > 0 ? idx[0] : 0;
  uint64_t const idx2 = idx.ndims() > 1 ? idx[1] : 0;
  uint64_t const idx3 = idx.ndims() > 2 ? idx[2] : 0;
  uint64_t const idx4 = idx.ndims() > 3 ? idx[3] : 0;

  // Set the current context index, enabling the user to query the index of
  // their collection element
  auto const proxy = base->getProxy();
  IdxContextHolder::set(&idx,proxy);

  runnable::RunnableCollection<MsgT,UntypedCollection>::run(
    han, msg, ptr, from, member, idx1, idx2, idx3, idx4, event
  );

  // Clear the current index context
  IdxContextHolder::clear();
}

template <typename ColT, typename IndexT, typename MsgT>
/*static*/ void CollectionManager::collectionBcastHandler(MsgT* msg) {
  auto const col_msg = static_cast<CollectionMessage<ColT>*>(msg);
  auto const bcast_proxy = col_msg->getBcastProxy();
  auto const& untyped_proxy = bcast_proxy;
  auto const& group = envelopeGetGroup(msg->env);
  auto const& cur_epoch = theMsg()->getEpoch();
  auto const& msg_epoch = envelopeGetEpoch(msg->env);
  theMsg()->pushEpoch(cur_epoch);
  vt_debug_print(
    vrt_coll, node,
    "collectionBcastHandler: bcast_proxy={:x}, han={}, bcast epoch={:x}, "
    "epoch={:x}, msg epoch={:x}, group={}, default group={}\n",
    bcast_proxy, col_msg->getVrtHandler(), col_msg->getBcastEpoch(),
    cur_epoch, msg_epoch, group, default_group
  );
  auto elm_holder = theCollection()->findElmHolder<ColT,IndexT>(bcast_proxy);
  if (elm_holder) {
    auto const handler = col_msg->getVrtHandler();
    auto const member = col_msg->getMember();
    vt_debug_print(
      vrt_coll, node,
      "broadcast apply: size={}\n", elm_holder->numElements()
    );
    elm_holder->foreach([col_msg,msg,handler,member](
      IndexT const& idx, CollectionBase<ColT,IndexT>* base
    ) {
      vt_debug_print(
        vrt_coll, node,
        "broadcast: apply to element: epoch={}, bcast_epoch={}\n",
        msg->bcast_epoch_, base->cur_bcast_epoch_
      );
      vtAssert(base != nullptr, "Must be valid pointer");
      base->cur_bcast_epoch_++;

      #if vt_check_enabled(lblite)
        vt_debug_print(
          vrt_coll, node,
          "broadcast: apply to element: instrument={}\n",
          msg->lbLiteInstrument()
        );
        if (msg->lbLiteInstrument()) {
          recordStats(base, msg);
          auto& stats = base->getStats();
          stats.startTime();
        }

        // Set the current context (element ID) that is executing (having a message
        // delivered). This is used for load balancing to build the communication
        // graph
        auto const perm_elm_id = base->getElmID();
        auto const temp_elm_id = base->getTempID();
        auto const perm_prev_elm = theCollection()->getCurrentContextPerm();
        auto const temp_prev_elm = theCollection()->getCurrentContextTemp();

        theCollection()->setCurrentContext(perm_elm_id, temp_elm_id);

        vt_debug_print(
          vrt_coll, node,
          "collectionBcastHandler: current context: perm={}, temp={}\n",
          perm_elm_id, temp_elm_id
        );

        std::unique_ptr<messaging::Listener> listener =
          std::make_unique<balance::LBListener>(
            [&](NodeType dest, MsgSizeType size, bool bcast){
              auto& stats = base->getStats();
              stats.recvToNode(dest, perm_elm_id, temp_elm_id, size, bcast);
            }
          );
        theMsg()->addSendListener(std::move(listener));
      #endif

      // be very careful here, do not touch `base' after running the active
      // message because it might have migrated out and be invalid
      auto const from = col_msg->getFromNode();
      trace::TraceEventIDType trace_event = trace::no_trace_event;
      #if vt_check_enabled(trace_enabled)
        trace_event = col_msg->getFromTraceEvent();
      #endif
      collectionAutoMsgDeliver<ColT,IndexT,MsgT,typename MsgT::UserMsgType>(
        msg,base,handler,member,from,trace_event
      );

      #if vt_check_enabled(lblite)
        theMsg()->clearListeners();

        // Unset the element ID context
        theCollection()->setCurrentContext(perm_prev_elm, temp_prev_elm);

        if (msg->lbLiteInstrument()) {
          auto& stats = base->getStats();
          stats.stopTime();
        }
      #endif
    });
  }
  /*
   *  Buffer the broadcast message for later delivery (elements that migrate
   *  in), inserted elements, etc.
   */
  auto col_holder = theCollection()->findColHolder<ColT,IndexT>(untyped_proxy);
  if (!col_holder->is_static_) {
    /*
     * @todo: buffer the broadcasts only when needed and clean up appropriately
     */
    // auto const& epoch = msg->bcast_epoch_;
    // theCollection()->bufferBroadcastMsg<ColT>(untyped_proxy, epoch, msg);
  }
  /*
   *  Termination: consume for default epoch for correct termination: on the
   *  other end the sender produces p units for each broadcast to the default
   *  group
   */
  vt_debug_print(
    vrt_coll, node,
    "collectionBcastHandler: (consume) bcast_proxy={:x}, han={}, bcast epoch={:x}, "
    "epoch={:x}, msg epoch={:x}, group={}, default group={}\n",
    bcast_proxy, col_msg->getVrtHandler(), col_msg->getBcastEpoch(),
    cur_epoch, msg_epoch, group, default_group
  );
  theMsg()->popEpoch(cur_epoch);
}

template <typename ColT, typename MsgT>
void CollectionManager::bufferBroadcastMsg(
  VirtualProxyType const& proxy, EpochType const& epoch, MsgT* msg
) {
  auto proxy_iter = details::Broadcasts<ColT>::m_.find(proxy);
  if (proxy_iter == details::Broadcasts<ColT>::m_.end()) {
    if (details::Broadcasts<ColT>::m_.size() == 0) {
      cleanup_fns_[proxy].push_back([]{ details::Broadcasts<ColT>::m_.clear(); });
    }
    details::Broadcasts<ColT>::m_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(proxy),
      std::forward_as_tuple(
        std::unordered_map<EpochType,CollectionMessage<ColT>*>{{epoch,msg}}
      )
    );
  } else {
    auto epoch_iter = proxy_iter->second.find(epoch);
    if (epoch_iter == proxy_iter->second.end()) {
      proxy_iter->second.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(epoch),
        std::forward_as_tuple(msg)
      );
    } else {
      epoch_iter->second = msg;
    }
  }
}

template <typename ColT>
void CollectionManager::clearBufferedBroadcastMsg(
  VirtualProxyType const& proxy, EpochType const& epoch
) {
  auto proxy_iter = details::Broadcasts<ColT>::m_.find(proxy);
  if (proxy_iter != details::Broadcasts<ColT>::m_.end()) {
    auto epoch_iter = proxy_iter->second.find(epoch);
    if (epoch_iter != proxy_iter->second.end()) {
      proxy_iter->second.erase(epoch_iter);
    }
  }
}

template <typename ColT, typename MsgT>
CollectionMessage<ColT>* CollectionManager::getBufferedBroadcastMsg(
  VirtualProxyType const& proxy, EpochType const& epoch
) {
  auto proxy_iter = details::Broadcasts<ColT>::m_.find(proxy);
  if (proxy_iter != details::Broadcasts<ColT>::m_.end()) {
    auto epoch_iter = proxy_iter->second.find(epoch);
    if (epoch_iter != proxy_iter->second.end()) {
      return proxy_iter->second->second;
    }
  }
  return nullptr;
}

template <typename>
/*static*/ void CollectionManager::collectionGroupFinishedHan(
  CollectionGroupMsg* msg
) {
  auto const& proxy = msg->getProxy();
  theCollection()->addToState(proxy, BufferReleaseEnum::AfterGroupReady);
  theCollection()->triggerReadyOps(proxy, BufferTypeEnum::Reduce);
}

template <typename>
/*static*/ void CollectionManager::collectionFinishedHan(
  CollectionConsMsg* msg
) {
  auto const& proxy = msg->proxy;
  theCollection()->constructed_.insert(proxy);
  theCollection()->addToState(proxy, BufferReleaseEnum::AfterFullyConstructed);
  vt_debug_print(
    vrt_coll, node,
    "addToState: proxy={:x}, AfterCons\n", proxy
  );
  theCollection()->triggerReadyOps(proxy, BufferTypeEnum::Broadcast);
  theCollection()->triggerReadyOps(proxy, BufferTypeEnum::Send);
  theCollection()->triggerReadyOps(proxy, BufferTypeEnum::Reduce);
}

template <typename>
/*static*/ void CollectionManager::collectionGroupReduceHan(
  CollectionGroupMsg* msg
) {
  vt_debug_print(
    vrt_coll, node,
    "collectionGroupReduceHan: proxy={:x}, root={}, group={}\n",
    msg->proxy, msg->isRoot(), msg->getGroup()
  );
  if (msg->isRoot()) {
    auto nmsg = makeMessage<CollectionGroupMsg>(*msg);
    theMsg()->markAsCollectionMessage(nmsg);
    theMsg()->broadcastMsg<CollectionGroupMsg,collectionGroupFinishedHan>(
      nmsg.get()
    );
  }
}

template <typename ColT, typename IndexT, typename MsgT>
/*static*/ void CollectionManager::collectionMsgTypedHandler(MsgT* msg) {
  auto const col_msg = static_cast<CollectionMessage<ColT>*>(msg);
  auto const entity_proxy = col_msg->getProxy();
  auto const cur_epoch = theMsg()->getEpochContextMsg(msg);
  auto const& col = entity_proxy.getCollectionProxy();
  auto const& elm = entity_proxy.getElementProxy();
  auto const& idx = elm.getIndex();
  auto elm_holder = theCollection()->findElmHolder<ColT, IndexT>(col);

  bool const exists = elm_holder->exists(idx);

  vt_debug_print(
    vrt_coll, node,
    "collectionMsgTypedHandler: exists={}, idx={}, cur_epoch={:x}\n",
    exists, idx, cur_epoch
  );

  vtAssertInfo(exists, "Proxy must exist", cur_epoch, idx);

  auto& inner_holder = elm_holder->lookup(idx);

  auto const sub_handler = col_msg->getVrtHandler();
  auto const member = col_msg->getMember();
  auto const col_ptr = inner_holder.getCollection();

  vt_debug_print(
    vrt_coll, node,
    "collectionMsgTypedHandler: sub_handler={}\n", sub_handler
  );

  vtAssertInfo(
    col_ptr != nullptr, "Must be valid pointer",
    sub_handler, member, cur_epoch, idx, exists
  );


  #if vt_check_enabled(lblite)
    vt_debug_print(
      vrt_coll, node,
      "collectionMsgTypedHandler: receive msg: instrument={}\n",
      col_msg->lbLiteInstrument()
    );
    if (col_msg->lbLiteInstrument()) {
      recordStats(col_ptr, msg);
      auto& stats = col_ptr->getStats();
      stats.startTime();
    }

    // Set the current context (element ID) that is executing (having a message
    // delivered). This is used for load balancing to build the communication
    // graph
    auto const perm_elm_id = col_ptr->getElmID();
    auto const temp_elm_id = col_ptr->getTempID();
    auto const perm_prev_elm = theCollection()->getCurrentContextPerm();
    auto const temp_prev_elm = theCollection()->getCurrentContextTemp();

    theCollection()->setCurrentContext(perm_elm_id, temp_elm_id);

    vt_debug_print(
      vrt_coll, node,
      "collectionMsgTypedHandler: current context: perm={}, temp={}\n",
      perm_elm_id, temp_elm_id
    );

    std::unique_ptr<messaging::Listener> listener =
      std::make_unique<balance::LBListener>(
        [&](NodeType dest, MsgSizeType size, bool bcast){
          auto& stats = col_ptr->getStats();
          stats.recvToNode(dest, perm_elm_id, temp_elm_id, size, bcast);
        }
      );
    theMsg()->addSendListener(std::move(listener));
  #endif

  // Dispatch the handler after pushing the contextual epoch
  theMsg()->pushEpoch(cur_epoch);
  auto const from = col_msg->getFromNode();

  trace::TraceEventIDType trace_event = trace::no_trace_event;
  #if vt_check_enabled(trace_enabled)
    trace_event = col_msg->getFromTraceEvent();
  #endif
  collectionAutoMsgDeliver<ColT,IndexT,MsgT,typename MsgT::UserMsgType>(
    msg,col_ptr,sub_handler,member,from,trace_event
  );
  theMsg()->popEpoch(cur_epoch);

  #if vt_check_enabled(lblite)
    theMsg()->clearListeners();

    theCollection()->setCurrentContext(perm_prev_elm, temp_prev_elm);

    if (col_msg->lbLiteInstrument()) {
      auto& stats = col_ptr->getStats();
      stats.stopTime();
    }
  #endif
}

template <typename ColT, typename MsgT>
/*static*/ void CollectionManager::recordStats(ColT* col_ptr, MsgT* msg) {
  auto const pto = col_ptr->getElmID();
  auto const tto = col_ptr->getTempID();
  auto const pfrom = msg->getElm();
  auto const tfrom = msg->getElmTemp();
  auto& stats = col_ptr->getStats();
  auto const msg_size = serialization::MsgSizer<MsgT>::get(msg);
  auto const cat = msg->getCat();
  vt_debug_print(
    vrt_coll, node,
    "recordStats: receive msg: perm(to={}, from={}), temp(to={}, from={})"
    " no={}, size={}, category={}\n",
    pto, pfrom, tto, tfrom, balance::no_element_id, msg_size,
    static_cast<typename std::underlying_type<balance::CommCategory>::type>(cat)
  );
  if (
    cat == balance::CommCategory::SendRecv or
    cat == balance::CommCategory::Broadcast
  ) {
    vtAssert(pfrom != balance::no_element_id, "Must not be no element ID");
    bool bcast = cat == balance::CommCategory::SendRecv ? false : true;
    stats.recvObjData(pto, tto, pfrom, tfrom, msg_size, bcast);
  } else if (
    cat == balance::CommCategory::NodeToCollection or
    cat == balance::CommCategory::NodeToCollectionBcast
  ) {
    bool bcast = cat == balance::CommCategory::NodeToCollection ? false : true;
    auto nfrom = msg->getFromNode();
    stats.recvFromNode(pto, tto, nfrom, msg_size, bcast);
  }
}


template <typename ColT, typename IndexT>
/*static*/ void CollectionManager::collectionMsgHandler(BaseMessage* msg) {
  return collectionMsgTypedHandler<ColT,IndexT,CollectionMessage<ColT>>(
    static_cast<CollectionMessage<ColT>*>(msg)
  );
}

template <typename ColT, typename IndexT, typename MsgT>
/*static*/ void CollectionManager::broadcastRootHandler(MsgT* msg) {
  theCollection()->broadcastFromRoot<ColT,IndexT,MsgT>(msg);
}

template <typename ColT, typename IndexT, typename MsgT>
messaging::PendingSend CollectionManager::broadcastFromRoot(MsgT* raw_msg) {
  auto msg = promoteMsg(raw_msg);

  // broadcast to all nodes
  auto const& this_node = theContext()->getNode();
  auto const& proxy = msg->getBcastProxy();
  auto elm_holder = theCollection()->findElmHolder<ColT,IndexT>(proxy);
  auto const bcast_node = VirtualProxyBuilder::getVirtualNode(proxy);

  vtAssert(elm_holder != nullptr, "Must have elm holder");
  vtAssert(this_node == bcast_node, "Must be the bcast node");

  auto const bcast_epoch = elm_holder->cur_bcast_epoch_++;
  auto const cur_epoch = theMsg()->getEpochContextMsg(msg);
  theMsg()->pushEpoch(cur_epoch);

  msg->setBcastEpoch(bcast_epoch);

  vt_debug_print(
    vrt_coll, node,
    "broadcastFromRoot: proxy={:x}, epoch={}, han={}\n",
    proxy, msg->getBcastEpoch(), msg->getVrtHandler()
  );

  auto const& group_ready = elm_holder->groupReady();
  auto const& use_group = elm_holder->useGroup();
  bool const send_group = group_ready && use_group;

  vt_debug_print(
    vrt_coll, node,
    "broadcastFromRoot: proxy={:x}, bcast epoch={}, han={}, group_ready={}, "
    "group_active={}, use_group={}, send_group={}, group={:x}, cur_epoch={:x}\n",
    proxy, msg->getBcastEpoch(), msg->getVrtHandler(),
    group_ready, send_group, use_group, send_group,
    use_group ? elm_holder->group() : default_group, cur_epoch
  );

  if (send_group) {
    auto const& group = elm_holder->group();
    envelopeSetGroup(msg->env, group);
  }

  theMsg()->markAsCollectionMessage(msg);
  auto ret = theMsg()->broadcastMsg<MsgT,collectionBcastHandler<ColT,IndexT>>(
    msg.get()
  );
  if (!send_group) {
    collectionBcastHandler<ColT,IndexT,MsgT>(msg.get());
  }

  theMsg()->popEpoch(cur_epoch);

  return ret;
}

template <
  typename MsgT,
  ActiveColMemberTypedFnType<MsgT,typename MsgT::CollectionType> f
>
messaging::PendingSend CollectionManager::broadcastMsg(
  CollectionProxyWrapType<typename MsgT::CollectionType> const& proxy,
  MsgT *msg, bool instrument
) {
  using ColT = typename MsgT::CollectionType;
  return broadcastMsg<MsgT,ColT,f>(proxy,msg,instrument);
}

template <
  typename MsgT,
  ActiveColTypedFnType<MsgT,typename MsgT::CollectionType> *f
>
messaging::PendingSend CollectionManager::broadcastMsg(
  CollectionProxyWrapType<typename MsgT::CollectionType> const& proxy,
  MsgT *msg, bool instrument
) {
  using ColT = typename MsgT::CollectionType;
  return broadcastMsg<MsgT,ColT,f>(proxy,msg,instrument);
}

template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
CollectionManager::IsColMsgType<MsgT>
CollectionManager::broadcastMsg(
  CollectionProxyWrapType<ColT> const& proxy, MsgT *msg,
  bool instrument
) {
  return broadcastMsgImpl<MsgT,ColT,f>(proxy,msg,instrument);
}

template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
CollectionManager::IsNotColMsgType<MsgT>
CollectionManager::broadcastMsg(
  CollectionProxyWrapType<ColT> const& proxy, MsgT *msg, bool instrument
) {
  auto const& h = auto_registry::makeAutoHandlerCollection<ColT,MsgT,f>(msg);
  return broadcastNormalMsg<MsgT,ColT>(proxy,msg,h,false,instrument);
}

template <
  typename MsgT,
  typename ColT,
  ActiveColMemberTypedFnType<MsgT,ColT> f
>
CollectionManager::IsColMsgType<MsgT>
CollectionManager::broadcastMsg(
  CollectionProxyWrapType<ColT> const& proxy, MsgT *msg,
  bool instrument
 ) {
  return broadcastMsgImpl<MsgT,ColT,f>(proxy,msg,instrument);
}

template <
  typename MsgT,
  typename ColT,
  ActiveColMemberTypedFnType<MsgT,ColT> f
>
CollectionManager::IsNotColMsgType<MsgT>
CollectionManager::broadcastMsg(
  CollectionProxyWrapType<ColT> const& proxy, MsgT *msg, bool instrument
) {
  auto const& h = auto_registry::makeAutoHandlerCollectionMem<ColT,MsgT,f>(msg);
  return broadcastNormalMsg<MsgT,ColT>(proxy,msg,h,true,instrument);
}

template <
  typename MsgT,
  typename ColT,
  ActiveColMemberTypedFnType<MsgT,ColT> f
>
messaging::PendingSend CollectionManager::broadcastMsgImpl(
  CollectionProxyWrapType<ColT> const& proxy, MsgT *const msg, bool inst
) {
  // register the user's handler
  auto const& h = auto_registry::makeAutoHandlerCollectionMem<ColT,MsgT,f>(msg);
  return broadcastMsgUntypedHandler<MsgT>(proxy,msg,h,true,inst);
}

template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
messaging::PendingSend CollectionManager::broadcastMsgImpl(
  CollectionProxyWrapType<ColT> const& proxy, MsgT *const msg, bool inst
) {
  // register the user's handler
  auto const& h = auto_registry::makeAutoHandlerCollection<ColT,MsgT,f>(msg);
  return broadcastMsgUntypedHandler<MsgT>(proxy,msg,h,false,inst);
}

template <typename MsgT, typename ColT>
CollectionManager::IsColMsgType<MsgT>
CollectionManager::broadcastMsgWithHan(
  CollectionProxyWrapType<ColT> const& proxy, MsgT *msg,
  HandlerType const& h, bool const mem, bool inst
) {
  using IdxT = typename ColT::IndexType;
  return broadcastMsgUntypedHandler<MsgT,ColT,IdxT>(proxy,msg,h,mem,inst);
}

template <typename MsgT, typename ColT>
CollectionManager::IsNotColMsgType<MsgT>
CollectionManager::broadcastMsgWithHan(
  CollectionProxyWrapType<ColT> const& proxy, MsgT *msg,
  HandlerType const& h, bool const mem, bool inst
) {
  return broadcastNormalMsg<MsgT,ColT>(proxy,msg,h,mem,inst);
}

template <typename MsgT, typename ColT>
messaging::PendingSend CollectionManager::broadcastNormalMsg(
  CollectionProxyWrapType<ColT> const& proxy, MsgT *msg,
  HandlerType const& handler, bool const member,
  bool instrument
) {
  auto wrap_msg = makeMessage<ColMsgWrap<ColT,MsgT>>(*msg);
  return broadcastMsgUntypedHandler<ColMsgWrap<ColT,MsgT>,ColT>(
    proxy, wrap_msg.get(), handler, member, instrument
  );
}

template <typename MsgT, typename ColT, typename IdxT>
messaging::PendingSend CollectionManager::broadcastMsgUntypedHandler(
  CollectionProxyWrapType<ColT, IdxT> const& toProxy, MsgT *raw_msg,
  HandlerType const& handler, bool const member, bool instrument
) {
  auto const idx = makeVrtDispatch<MsgT,ColT>();
  auto const col_proxy = toProxy.getProxy();
  auto msg = promoteMsg(raw_msg);

  vt_debug_print(
    vrt_coll, node,
    "broadcastMsgUntypedHandler: msg={}, idx={}\n",
    print_ptr(msg.get()), idx
  );

  // save the user's handler in the message
  msg->setFromNode(theContext()->getNode());
  msg->setVrtHandler(handler);
  msg->setBcastProxy(col_proxy);
  msg->setMember(member);

# if vt_check_enabled(trace_enabled)
  // Create the trace creation event for the broadcast here to connect it a
  // higher semantic level
  auto reg_type = member ?
    auto_registry::RegistryTypeEnum::RegVrtCollectionMember :
    auto_registry::RegistryTypeEnum::RegVrtCollection;
  auto msg_size = vt::serialization::MsgSizer<MsgT>::get(msg.get());
  auto event = theMsg()->makeTraceCreationSend(
    msg, handler, reg_type, msg_size, true
  );
  msg->setFromTraceEvent(event);
# endif

# if vt_check_enabled(lblite)
  msg->setLBLiteInstrument(instrument);
  auto const temp_elm_id = getCurrentContextTemp();
  auto const perm_elm_id = getCurrentContextPerm();

  vt_debug_print(
    vrt_coll, node,
    "broadcasting msg: LB current elm context perm={}, temp={}\n",
    perm_elm_id, temp_elm_id
  );

  if (perm_elm_id != balance::no_element_id) {
    msg->setElm(perm_elm_id, temp_elm_id);
    msg->setCat(balance::CommCategory::Broadcast);
  } else {
    msg->setCat(balance::CommCategory::NodeToCollection);
  }
# endif

  auto const cur_epoch = theMsg()->setupEpochMsg(msg);

  return bufferOpOrExecute<ColT>(
    col_proxy,
    BufferTypeEnum::Broadcast,
    static_cast<BufferReleaseEnum>(AfterFullyConstructed | AfterMetaDataKnown),
    cur_epoch,
    [=]() -> messaging::PendingSend {
      auto const this_node = theContext()->getNode();
      auto const bnode = VirtualProxyBuilder::getVirtualNode(col_proxy);
      if (this_node != bnode) {
        vt_debug_print(
          vrt_coll, node,
          "broadcastMsgUntypedHandler: col_proxy={:x}, sending to root node={}, "
          "handler={}, cur_epoch={:x}\n",
          col_proxy, bnode, handler, cur_epoch
        );
        theMsg()->markAsCollectionMessage(msg);
        return theMsg()->sendMsg<MsgT,broadcastRootHandler<ColT,IdxT>>(
          bnode,msg.get()
        );
      } else {
        vt_debug_print(
          vrt_coll, node,
          "broadcasting msg to collection: msg={}, handler={}\n",
          print_ptr(msg.get()), handler
        );
        return broadcastFromRoot<ColT,IdxT,MsgT>(msg.get());
      }
    }
  );
}

template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
messaging::PendingSend CollectionManager::reduceMsgExpr(
  CollectionProxyWrapType<ColT> const& proxy,
  MsgT *const raw_msg, ReduceIdxFuncType<typename ColT::IndexType> expr_fn,
  ReduceStamp stamp, NodeType root
) {
  using IndexT = typename ColT::IndexType;

  auto msg = promoteMsg(raw_msg);

  vt_debug_print(
    vrt_coll, node,
    "reduceMsg: msg={}\n", print_ptr(raw_msg)
  );

  auto const col_proxy = proxy.getProxy();
  auto const cur_epoch = theMsg()->getEpochContextMsg(msg);

  return bufferOpOrExecute<ColT>(
    col_proxy,
    BufferTypeEnum::Reduce,
    static_cast<BufferReleaseEnum>(
      AfterFullyConstructed | AfterMetaDataKnown | AfterGroupReady
    ),
    cur_epoch,
    [=]() -> messaging::PendingSend {
      auto elm_holder = findElmHolder<ColT,IndexT>(col_proxy);

      std::size_t num_elms = 0;
      if (expr_fn == nullptr) {
        num_elms = elm_holder->numElements();
      } else {
        num_elms = elm_holder->numElementsExpr(expr_fn);
      }

      auto const root_node =
        root == uninitialized_destination ? default_collection_reduce_root_node :
        root;

      auto const group_ready = elm_holder->groupReady();
      auto const send_group = elm_holder->useGroup();
      auto const group = elm_holder->group();
      bool const use_group = group_ready && send_group;

      vtAssert(group_ready, "Must be ready");

      ReduceVirtualIDType reduce_id = std::make_tuple(stamp,col_proxy);

      auto stamp_iter = reduce_cur_stamp_.find(reduce_id);

      ReduceStamp cur_stamp = stamp;
      if (stamp == ReduceStamp{} && stamp_iter != reduce_cur_stamp_.end()) {
        cur_stamp = stamp_iter->second;
      }

      collective::reduce::Reduce* r = nullptr;
      if (use_group) {
        r = theGroup()->groupReducer(group);
      } else {
        r = theCollective()->getReducerVrtProxy(col_proxy);
      }

      auto ret_stamp = r->reduceImmediate<MsgT,f>(root_node, msg.get(), cur_stamp, num_elms);

      vt_debug_print(
        vrt_coll, node,
        "reduceMsg: col_proxy={:x}, num_elms={}\n",
        col_proxy, num_elms
      );

      if (stamp_iter == reduce_cur_stamp_.end()) {
        reduce_cur_stamp_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(reduce_id),
          std::forward_as_tuple(ret_stamp)
        );
      }

      vt_debug_print(
        vrt_coll, node,
        "reduceMsg: col_proxy={:x}, num_elms={}\n",
        col_proxy, num_elms
      );

      return messaging::PendingSend{nullptr};
    }
  );
}

template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
messaging::PendingSend CollectionManager::reduceMsg(
  CollectionProxyWrapType<ColT> const& proxy,
  MsgT *const msg, ReduceStamp stamp, NodeType root
) {
  return reduceMsgExpr<ColT,MsgT,f>(proxy,msg,nullptr,stamp,root);
}

template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
messaging::PendingSend CollectionManager::reduceMsg(
  CollectionProxyWrapType<ColT> const& proxy,
  MsgT *const msg, ReduceStamp stamp, typename ColT::IndexType const& idx
) {
  return reduceMsgExpr<ColT,MsgT,f>(proxy,msg,nullptr,stamp,idx);
}

template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
messaging::PendingSend CollectionManager::reduceMsgExpr(
  CollectionProxyWrapType<ColT> const& proxy,
  MsgT *const msg, ReduceIdxFuncType<typename ColT::IndexType> expr_fn,
  ReduceStamp stamp, typename ColT::IndexType const& idx
) {
  using IndexT = typename ColT::IndexType;
  auto const untyped_proxy = proxy.getProxy();
  auto constructed = constructed_.find(untyped_proxy) != constructed_.end();
  vtAssert(constructed, "Must be constructed");
  auto col_holder = findColHolder<ColT,IndexT>(untyped_proxy);
  auto max_idx = col_holder->max_idx;
  auto map_han = UniversalIndexHolder<>::getMap(untyped_proxy);
  auto insert_epoch = UniversalIndexHolder<>::insertGetEpoch(untyped_proxy);
  vtAssert(insert_epoch != no_epoch, "Epoch should be valid");
  bool const& is_functor =
    auto_registry::HandlerManagerType::isHandlerFunctor(map_han);
  auto_registry::AutoActiveMapType fn = nullptr;
  if (is_functor) {
    fn = auto_registry::getAutoHandlerFunctorMap(map_han);
  } else {
    fn = auto_registry::getAutoHandlerMap(map_han);
  }
  auto idx_non_const = idx;
  auto idx_non_const_ptr = &idx_non_const;
  auto const& mapped_node = fn(
    reinterpret_cast<vt::index::BaseIndex*>(idx_non_const_ptr),
    reinterpret_cast<vt::index::BaseIndex*>(&max_idx),
    theContext()->getNumNodes()
  );
  return reduceMsgExpr<ColT,MsgT,f>(proxy,msg,nullptr,stamp,mapped_node);
}

template <typename MsgT, typename ColT>
CollectionManager::IsNotColMsgType<MsgT>
CollectionManager::sendMsgWithHan(
  VirtualElmProxyType<ColT> const& proxy, MsgT *msg,
  HandlerType const& handler, bool const member
) {
  return sendNormalMsg<MsgT,ColT>(proxy,msg,handler,member);
}

template <typename MsgT, typename ColT>
CollectionManager::IsColMsgType<MsgT>
CollectionManager::sendMsgWithHan(
  VirtualElmProxyType<ColT> const& proxy, MsgT *msg,
  HandlerType const& handler, bool const member
) {
  using IdxT = typename ColT::IndexType;
  return sendMsgUntypedHandler<MsgT,ColT,IdxT>(proxy,msg,handler,member);
}

template <typename MsgT, typename ColT>
messaging::PendingSend CollectionManager::sendNormalMsg(
  VirtualElmProxyType<ColT> const& proxy, MsgT *msg,
  HandlerType const& handler, bool const member
) {
  auto wrap_msg = makeMessage<ColMsgWrap<ColT,MsgT>>(*msg);
  return sendMsgUntypedHandler<ColMsgWrap<ColT,MsgT>,ColT>(
    proxy, wrap_msg.get(), handler, member
  );
}

template <
  typename MsgT, ActiveColTypedFnType<MsgT,typename MsgT::CollectionType> *f
>
messaging::PendingSend CollectionManager::sendMsg(
  VirtualElmProxyType<typename MsgT::CollectionType> const& proxy, MsgT *msg
) {
  using ColT = typename MsgT::CollectionType;
  return sendMsg<MsgT,ColT,f>(proxy,msg);
}

template <
  typename MsgT,
  ActiveColMemberTypedFnType<MsgT,typename MsgT::CollectionType> f
>
messaging::PendingSend CollectionManager::sendMsg(
  VirtualElmProxyType<typename MsgT::CollectionType> const& proxy, MsgT *msg
) {
  using ColT = typename MsgT::CollectionType;
  return sendMsg<MsgT,ColT,f>(proxy,msg);
}

template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
CollectionManager::IsColMsgType<MsgT>
CollectionManager::sendMsg(
  VirtualElmProxyType<ColT> const& proxy, MsgT *msg
) {
  return sendMsgImpl<MsgT,ColT,f>(proxy,msg);
}

template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
CollectionManager::IsNotColMsgType<MsgT>
CollectionManager::sendMsg(
  VirtualElmProxyType<ColT> const& proxy, MsgT *msg
) {
  auto const& h = auto_registry::makeAutoHandlerCollection<ColT,MsgT,f>(msg);
  return sendNormalMsg<MsgT,ColT>(proxy,msg,h,false);
}

template <
  typename MsgT,
  typename ColT,
  ActiveColMemberTypedFnType<MsgT,ColT> f
>
CollectionManager::IsColMsgType<MsgT>
CollectionManager::sendMsg(
  VirtualElmProxyType<ColT> const& proxy, MsgT *msg
) {
  return sendMsgImpl<MsgT,ColT,f>(proxy,msg);
}

template <
  typename MsgT,
  typename ColT,
  ActiveColMemberTypedFnType<MsgT,ColT> f
>
CollectionManager::IsNotColMsgType<MsgT>
CollectionManager::sendMsg(
  VirtualElmProxyType<ColT> const& proxy, MsgT *msg
) {
  auto const& h = auto_registry::makeAutoHandlerCollectionMem<ColT,MsgT,f>(msg);
  return sendNormalMsg<MsgT,ColT>(proxy,msg,h,true);
}

template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
messaging::PendingSend CollectionManager::sendMsgImpl(
  VirtualElmProxyType<ColT> const& proxy, MsgT *msg
) {
  auto const& h = auto_registry::makeAutoHandlerCollection<ColT,MsgT,f>(msg);
  return sendMsgUntypedHandler<MsgT>(proxy,msg,h,false);
}

template <
  typename MsgT,
  typename ColT,
  ActiveColMemberTypedFnType<MsgT,typename MsgT::CollectionType> f
>
messaging::PendingSend CollectionManager::sendMsgImpl(
  VirtualElmProxyType<ColT> const& proxy, MsgT *msg
) {
  auto const& h = auto_registry::makeAutoHandlerCollectionMem<ColT,MsgT,f>(msg);
  return sendMsgUntypedHandler<MsgT>(proxy,msg,h,true);
}

template <typename MsgT, typename ColT, typename IdxT>
messaging::PendingSend CollectionManager::sendMsgUntypedHandler(
  VirtualElmProxyType<ColT> const& toProxy, MsgT *raw_msg,
  HandlerType const& handler, bool const member,
  bool imm_context
) {
  auto const& col_proxy = toProxy.getCollectionProxy();
  auto const& elm_proxy = toProxy.getElementProxy();

  auto msg = promoteMsg(raw_msg);

# if vt_check_enabled(lblite)
  msg->setLBLiteInstrument(true);

  auto const temp_elm_id = getCurrentContextTemp();
  auto const perm_elm_id = getCurrentContextPerm();

  vt_debug_print(
    vrt_coll, node,
    "sending msg: LB current elm context perm={}, temp={}\n",
    perm_elm_id, temp_elm_id
  );

  if (perm_elm_id != balance::no_element_id) {
    msg->setElm(perm_elm_id, temp_elm_id);
    msg->setCat(balance::CommCategory::SendRecv);
  } else {
    msg->setCat(balance::CommCategory::NodeToCollection);
  }
# endif

# if vt_check_enabled(trace_enabled)
  // Create the trace creation event here to connect it a higher semantic
  // level. Do it in the imm_context so we see the send event when the user
  // actually invokes send on the proxy (not outside the event that actually
  // sent it)
  auto reg_type = member ?
    auto_registry::RegistryTypeEnum::RegVrtCollectionMember :
    auto_registry::RegistryTypeEnum::RegVrtCollection;
  auto msg_size = vt::serialization::MsgSizer<MsgT>::get(msg.get());
  auto event = theMsg()->makeTraceCreationSend(
    msg, handler, reg_type, msg_size, false
  );
  msg->setFromTraceEvent(event);
#endif

  auto const cur_epoch = theMsg()->setupEpochMsg(msg);
  msg->setFromNode(theContext()->getNode());
  msg->setVrtHandler(handler);
  msg->setProxy(toProxy);
  msg->setMember(member);

  auto idx = elm_proxy.getIndex();
  vt_debug_print(
    vrt_coll, node,
    "sendMsgUntypedHandler: col_proxy={:x}, cur_epoch={:x}, idx={}, "
    "handler={}, imm_context={}\n",
    col_proxy, cur_epoch, idx, handler, imm_context
  );

  return schedule(
    msg, !imm_context, cur_epoch, [=]{
      bufferOpOrExecute<ColT>(
        col_proxy,
        BufferTypeEnum::Send,
        BufferReleaseEnum::AfterMetaDataKnown,
        cur_epoch,
        [=]() -> messaging::PendingSend {
          auto home_node = getMapped<ColT>(col_proxy, idx);
          // route the message to the destination using the location manager
          auto lm = theLocMan()->getCollectionLM<ColT, IdxT>(col_proxy);
          vtAssert(lm != nullptr, "LM must exist");
          theMsg()->markAsCollectionMessage(msg);
          lm->template routeMsgSerializeHandler<
            MsgT, collectionMsgTypedHandler<ColT,IdxT,MsgT>
          >(toProxy, home_node, msg);
          return messaging::PendingSend{nullptr};
        }
      );
    }
  );
}

template <typename ColT, typename IndexT>
bool CollectionManager::insertCollectionElement(
  VirtualPtrType<ColT, IndexT> vc, IndexT const& idx, IndexT const& max_idx,
  HandlerType const& map_han, VirtualProxyType const& proxy,
  bool const is_static, NodeType const& home_node, bool const& is_migrated_in,
  NodeType const& migrated_from
) {
  auto holder = findColHolder<ColT, IndexT>(proxy);

  vt_debug_print(
    vrt_coll, node,
    "insertCollectionElement: proxy={:x}, map_han={}, idx={}, max_idx={}\n",
    proxy, map_han, print_index(idx), print_index(max_idx)
  );

  if (holder == nullptr) {
    insertMetaCollection<ColT>(proxy,map_han,max_idx,is_static);
  }

  auto elm_holder = findElmHolder<ColT,IndexT>(proxy);
  auto const elm_exists = elm_holder->exists(idx);

  vt_debug_print(
    vrt_coll, node,
    "insertCollectionElement: elm_exists={}, proxy={:x}, idx={}\n",
    elm_exists, proxy, idx
  );

  if (elm_exists) {
    return false;
  }
  vtAssert(!elm_exists, "Must not exist at this point");

  auto const destroyed = elm_holder->isDestroyed();

  vt_debug_print(
    vrt_coll, node,
    "insertCollectionElement: destroyed={}, proxy={:x}, idx={}\n",
    destroyed, proxy, idx
  );

  if (!destroyed) {
    elm_holder->insert(idx, typename Holder<ColT, IndexT>::InnerHolder{
      std::move(vc), map_han, max_idx
    });

    if (is_migrated_in) {
      theLocMan()->getCollectionLM<ColT, IndexT>(proxy)->registerEntityMigrated(
        VrtElmProxy<ColT, IndexT>{proxy,idx}, home_node, migrated_from,
        CollectionManager::collectionMsgHandler<ColT, IndexT>
      );
      elm_holder->applyListeners(
        listener::ElementEventEnum::ElementMigratedIn, idx
      );
    } else {
      theLocMan()->getCollectionLM<ColT, IndexT>(proxy)->registerEntity(
        VrtElmProxy<ColT, IndexT>{proxy,idx}, home_node,
        CollectionManager::collectionMsgHandler<ColT, IndexT>
      );
      elm_holder->applyListeners(
        listener::ElementEventEnum::ElementCreated, idx
      );
    }
    return true;
  } else {
    return false;
  }
}

/*
 * Support constructing a VT collection in a fully distributed manner, with the
 * user supplying the pointers to the elements.
 */

template <typename ColT>
CollectionManager::CollectionProxyWrapType<ColT>
 CollectionManager::constructCollective(
  typename ColT::IndexType range, DistribConstructFn<ColT> cons_fn,
  TagType const& tag
) {
  auto const map_han = getDefaultMap<ColT>();
  return constructCollectiveMap<ColT>(range,cons_fn,map_han,tag);
}


template <
  typename ColT,  mapping::ActiveMapTypedFnType<typename ColT::IndexType> fn
>
CollectionManager::CollectionProxyWrapType<ColT, typename ColT::IndexType>
CollectionManager::constructCollective(
  typename ColT::IndexType range, DistribConstructFn<ColT> cons_fn,
  TagType const& tag
) {
  using IndexT = typename ColT::IndexType;
  auto const& map_han = auto_registry::makeAutoHandlerMap<IndexT, fn>();
  return constructCollectiveMap<ColT>(range,cons_fn,map_han,tag);
}


template <typename ColT>
CollectionManager::CollectionProxyWrapType<ColT>
CollectionManager::constructCollectiveMap(
  typename ColT::IndexType range, DistribConstructFn<ColT> user_construct_fn,
  HandlerType const& map_han, TagType const& tag
) {
  using IndexT         = typename ColT::IndexType;
  using TypedProxyType = CollectionProxyWrapType<ColT>;

  auto const num_nodes = theContext()->getNumNodes();

  // Register the collection mapping function
  // auto const& map_han = auto_registry::makeAutoHandlerMap<IndexT, fn>();

  // Create a new distributed proxy, ordered wrt the input tag
  auto const& proxy = makeDistProxy<>(tag);

  // Initialize the typed proxy for the user interface, returned from this fn
  auto const typed_proxy = TypedProxyType{proxy};

  // For now, the distributed SPMD constructed collection must be statically
  // sized, not dynamically insertable to use this constructed methodology.
  auto const& is_static = ColT::isStaticSized();
  vtAssertInfo(
    is_static, "Distributed collection construct must be statically sized",
    is_static, tag, map_han
  );

  // Invoke getCollectionLM() to create a new location manager instance for this
  // collection
  theLocMan()->getCollectionLM<ColT, IndexT>(proxy);

  vt_debug_print(
    vrt_coll, node,
    "construct (dist): proxy={:x}, is_static={}\n",
    proxy, is_static
  );

  // Insert action on cleanup for this collection
  theCollection()->addCleanupFn<ColT>(proxy);

  // Start the local collection initiation process, lcoal meta-info about the
  // collection. Insert epoch is `no_epoch` because dynamic insertions are not
  // allowed when using SPMD distributed construction currently
  insertCollectionInfo(proxy, map_han, no_epoch);

  // Total count across the statically sized collection
  auto const num_elms = range.getSize();

  // Get the handler function
  auto fn = auto_registry::getHandlerMap(map_han);

  // Walk through the index range with the mapping function and invoke the
  // construct function (parameter to this fn) for local elements, populating
  // the holder
  range.foreach([&](IndexT cur_idx) mutable {
    using BaseIdxType      = vt::index::BaseIndex;
    using VirtualElmPtr    = VirtualPtrType<ColT,IndexT>;
    using IdxContextHolder = InsertContextHolder<IndexT>;

    vt_debug_print_verbose(
      vrt_coll, node,
      "construct (dist): foreach: map: cur_idx={}, index range={}\n",
      cur_idx.toString(), range.toString()
    );

    auto const cur = static_cast<BaseIdxType*>(&cur_idx);
    auto const max = static_cast<BaseIdxType*>(&range);

    auto mapped_node = fn(cur, max, num_nodes);

    vt_debug_print_verbose(
      vrt_coll, node,
      "construct (dist): foreach: cur_idx={}, mapped_node={}\n",
      cur_idx.toString(), mapped_node
    );

    if (theContext()->getNode() == mapped_node) {
      // // Check the current context index, asserting that it's nullptr (there can
      // // only be one live creation context at time)
      // auto const ctx_idx = IdxContextHolder::index();

      // vtAssert(ctx_idx == nullptr, "Context index must not be set");

      // Set the current context index to `cur_idx`. This enables the user to
      // query the index of their collection element in the constructor, which
      // is often very handy
      IdxContextHolder::set(&cur_idx,proxy);

      // Invoke the user's construct function with a single argument---the index
      // of element being constructed
      VirtualElmPtr elm_ptr = user_construct_fn(cur_idx);

      vt_debug_print_verbose(
        vrt_coll, node,
        "construct (dist): ptr={}\n", print_ptr(elm_ptr.get())
      );

      // Through the attorney, setup all the properties on the newly constructed
      // collection element: index, proxy, number of elements. Note: because of
      // how the constructor works, the index is not currently available through
      // "getIndex"
      CollectionTypeAttorney::setup(elm_ptr.get(), num_elms, cur_idx, proxy);

      // Insert the element into the managed holder for elements
      insertCollectionElement<ColT>(
        std::move(elm_ptr), cur_idx, range, map_han, proxy, true, mapped_node
      );

      // Clear the current index context
      IdxContextHolder::clear();

      vt_debug_print_verbose(
        vrt_coll, node,
        "construct (dist): new local elm: num_elm={}, proxy={:x}, cur_idx={}\n",
        num_elms, proxy, cur_idx.toString()
      );
    }
  });

  // Insert the meta-data for this new collection
  insertMetaCollection<ColT>(proxy, map_han, range, is_static);

  // Reduce construction of the distributed collection
  reduceConstruction<ColT>(proxy);

  // Construct a underlying group for the collection
  groupConstruction<ColT>(proxy, is_static);

  vt_debug_print(
    vrt_coll, node,
    "constructCollectiveMap: entering wait for constructed_\n"
  );

  // Wait for construction to finish before we release control to the user; this
  // ensures that other parts of the system do not migrate elements until the
  // group construction is complete
  theSched()->runSchedulerWhile([this, &proxy]{
    return constructed_.find(proxy) == constructed_.end();
  });

  vt_debug_print(
    vrt_coll, node,
    "constructCollectiveMap: proxy in constructed finished\n"
  );

  return typed_proxy;
}

template <typename>
VirtualProxyType CollectionManager::makeDistProxy(TagType const& tag) {
  static constexpr VirtualIDType first_dist_id = 0;

  // Get the next distributed ID for a given tag
  auto id_iter = dist_tag_id_.find(tag);
  if (id_iter == dist_tag_id_.end()) {
    dist_tag_id_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(tag),
      std::forward_as_tuple(first_dist_id)
    );
    id_iter = dist_tag_id_.find(tag);
  }

  vtAssertInfo(
    id_iter != dist_tag_id_.end(), "Dist tag iter must not be end",
    tag, first_dist_id
  );

  VirtualIDType const new_dist_id = id_iter->second++;

  auto const& this_node = theContext()->getNode();
  bool const& is_collection = true;
  bool const& is_migratable = true;
  bool const& is_distributed = true;

  // Create the new proxy with the `new_dist_id`
  auto const proxy = VirtualProxyBuilder::createProxy(
    new_dist_id, this_node, is_collection, is_migratable, is_distributed
  );

  vt_debug_print(
    vrt_coll, node,
    "makeDistProxy: node={}, new_dist_id={}, proxy={:x}\n",
    this_node, new_dist_id, proxy
  );

  return proxy;
}

/* end SPMD distributed collection support */


template <typename ColT>
void CollectionManager::staticInsertColPtr(
  VirtualProxyType proxy, typename ColT::IndexType idx,
  std::unique_ptr<ColT> ptr
) {
  using IndexT = typename ColT::IndexType;
  using BaseIdxType = vt::index::BaseIndex;

  auto map_han = UniversalIndexHolder<>::getMap(proxy);
  auto holder = findColHolder<ColT, IndexT>(proxy);
  auto range = holder->max_idx;
  auto const num_elms = range.getSize();
  auto fn = auto_registry::getHandlerMap(map_han);
  auto const num_nodes = theContext()->getNumNodes();
  auto const cur = static_cast<BaseIdxType*>(&idx);
  auto const max = static_cast<BaseIdxType*>(&range);
  auto const home_node = fn(cur, max, num_nodes);

  // Through the attorney, setup all the properties on the newly constructed
  // collection element: index, proxy, number of elements. Note: because of
  // how the constructor works, the index is not currently available through
  // "getIndex"
  CollectionTypeAttorney::setup(ptr.get(), num_elms, idx, proxy);

  VirtualPtrType<ColT, IndexT> col_ptr(
    static_cast<CollectionBase<ColT, IndexT>*>(ptr.release())
  );

  // Insert the element into the managed holder for elements
  insertCollectionElement<ColT>(
    std::move(col_ptr), idx, range, map_han, proxy, true, home_node
  );
}

template <typename ColT, typename... Args>
void CollectionManager::staticInsert(
  VirtualProxyType proxy, typename ColT::IndexType idx, Args&&... args
) {
  using IndexT           = typename ColT::IndexType;
  using IdxContextHolder = InsertContextHolder<IndexT>;

  auto tuple = std::make_tuple(std::forward<Args>(args)...);

  auto holder = findColHolder<ColT, IndexT>(proxy);

  auto range = holder->max_idx;
  auto const num_elms = range.getSize();

  // Set the current context index to `idx`
  IdxContextHolder::set(&idx,proxy);

  #if vt_check_enabled(detector) && vt_check_enabled(cons_multi_idx)
    auto elm_ptr = DerefCons::derefTuple<ColT, IndexT, decltype(tuple)>(
      num_elms, idx, &tuple
    );
  #else
    static constexpr auto num_args = std::tuple_size<decltype(tuple)>::value;
    auto elm_ptr = CollectionManager::runConstructor<ColT, IndexT>(
      num_elms, idx, &tuple, std::make_index_sequence<num_args>{}
    );
  #endif

  // Clear the current index context
  IdxContextHolder::clear();

  vt_debug_print_verbose(
    vrt_coll, node,
    "construct (staticInsert): ptr={}\n", print_ptr(elm_ptr.get())
  );

  std::unique_ptr<ColT> col_ptr(static_cast<ColT*>(elm_ptr.release()));

  staticInsertColPtr<ColT>(proxy, idx, std::move(col_ptr));
}

template <
  typename ColT, mapping::ActiveMapTypedFnType<typename ColT::IndexType> fn
>
InsertToken<ColT> CollectionManager::constructInsert(
  typename ColT::IndexType range, TagType const& tag
) {
  using IndexT = typename ColT::IndexType;
  auto const& map_han = auto_registry::makeAutoHandlerMap<IndexT, fn>();
  return constructInsertMap<ColT>(range, map_han, tag);
}

template <typename ColT>
InsertToken<ColT> CollectionManager::constructInsert(
  typename ColT::IndexType range, TagType const& tag
) {
  auto const map_han = getDefaultMap<ColT>();
  return constructInsertMap<ColT>(range, map_han, tag);
}

template <typename ColT>
InsertToken<ColT> CollectionManager::constructInsertMap(
  typename ColT::IndexType range, HandlerType const& map_han, TagType const& tag
) {
  using IndexT         = typename ColT::IndexType;

  // Create a new distributed proxy, ordered wrt the input tag
  auto const& proxy = makeDistProxy<>(tag);

  // For now, the staged insert collection must be statically sized. It is *not*
  // dynamically insertable!
  auto const& is_static = ColT::isStaticSized();
  vtAssertInfo(
    is_static, "Staged insert construction must be statically sized",
    is_static, tag, map_han
  );

  // Invoke getCollectionLM() to create a new location manager instance for this
  // collection
  theLocMan()->getCollectionLM<ColT, IndexT>(proxy);

  // Start the local collection initiation process, lcoal meta-info about the
  // collection. Insert epoch is `no_epoch` because dynamic insertions are not
  // allowed when using SPMD distributed construction currently
  insertCollectionInfo(proxy, map_han, no_epoch);

  // Insert the meta-data for this new collection
  insertMetaCollection<ColT>(proxy, map_han, range, is_static);

  // Insert action on cleanup for this collection
  theCollection()->addCleanupFn<ColT>(proxy);

  return InsertToken<ColT>{proxy};
}

template <typename ColT>
CollectionManager::CollectionProxyWrapType<ColT>
CollectionManager::finishedInsert(InsertToken<ColT>&& token) {
  using TypedProxyType = CollectionProxyWrapType<ColT>;

  InsertToken<ColT>&& tok = std::move(token);

  auto const& proxy = tok.getProxy();

  // Initialize the typed proxy for the user interface, returned from this fn
  auto const typed_proxy = TypedProxyType{proxy};

  // Reduce construction of the distributed collection
  reduceConstruction<ColT>(proxy);

  // Construct a underlying group for the collection
  groupConstruction<ColT>(proxy, true);

  return typed_proxy;
}

template <typename ColT>
/*static*/ HandlerType CollectionManager::getDefaultMap() {
  using ParamT = typename DefaultMap<ColT>::MapParamPackType;
  return getDefaultMapImpl<ColT,ParamT>(ParamT{});
}

template <typename ColT, typename ParamT, typename... Args>
/*static*/ HandlerType CollectionManager::getDefaultMapImpl(
  std::tuple<Args...>
) {
  using MapT = typename DefaultMap<ColT>::MapType;
  return auto_registry::makeAutoHandlerFunctorMap<MapT,Args...>();
}

template <typename IndexT>
/*static*/ IndexT* CollectionManager::queryIndexContext() {
  using IdxContextHolder = InsertContextHolder<IndexT>;
  return IdxContextHolder::index();
}

template <typename IndexT>
/*static*/ VirtualProxyType CollectionManager::queryProxyContext() {
  using IdxContextHolder = InsertContextHolder<IndexT>;
  return IdxContextHolder::proxy();
}

template <typename IndexT>
/*static*/ bool CollectionManager::hasContext() {
  using IdxContextHolder = InsertContextHolder<IndexT>;
  return IdxContextHolder::hasContext();
}

template <typename ColT, typename... Args>
/*static*/ void CollectionManager::insertMetaCollection(
  VirtualProxyType const& proxy, Args&&... args
) {
  using IndexType      = typename ColT::IndexType;
  using MetaHolderType = EntireHolder<ColT, IndexType>;
  using HolderType     = typename MetaHolderType::InnerHolder;

  // Create and insert the meta-data into the meta-collection holder
  auto holder = std::make_shared<HolderType>(std::forward<Args>(args)...);
  MetaHolderType::insert(proxy,holder);
  /*
   *  This is to ensure that the collection LM instance gets created so that
   *  messages can be forwarded properly
   */
  theLocMan()->getCollectionLM<ColT,IndexType>(proxy);
  vt_debug_print(
    vrt_coll, node,
    "addToState: proxy={:x}, AfterMeta\n", proxy
  );
  theCollection()->addToState(proxy, BufferReleaseEnum::AfterMetaDataKnown);
  theCollection()->triggerReadyOps(proxy, BufferTypeEnum::Send);
  theCollection()->triggerReadyOps(proxy, BufferTypeEnum::Broadcast);
  theCollection()->triggerReadyOps(proxy, BufferTypeEnum::Reduce);
}

template <typename ColT>
/*static*/ void CollectionManager::reduceConstruction(
  VirtualProxyType const& proxy
) {
  /*
   * Start a asynchronous reduction to coordinate operations that might depend
   * on construction completing (meta-data must be available, LM initialized,
   * etc.)
   */
  auto msg = makeMessage<CollectionConsMsg>(proxy);
  theMsg()->markAsCollectionMessage(msg);
  auto const& root = 0;
  vt_debug_print(
    vrt_coll, node,
    "reduceConstruction: invoke reduce: proxy={:x}\n", proxy
  );

  using collective::reduce::makeStamp;
  using collective::reduce::StrongUserID;

  auto stamp = makeStamp<StrongUserID>(proxy);
  auto r = theCollection()->reducer();
  auto cb = theCB()->makeBcast<CollectionConsMsg, collectionFinishedHan<void>>();
  r->reduce<collective::None>(root, msg.get(), cb, stamp);
}

template <typename ColT>
/*static*/ void CollectionManager::groupConstruction(
  VirtualProxyType const& proxy, bool immediate
) {
  if (immediate) {
    /*
     *  Create a new group for the collection that only contains the nodes for
     *  which elements exist. If the collection is static, this group will never
     *  change
     */

    using IndexT = typename ColT::IndexType;
    auto const elms = theCollection()->groupElementCount<ColT,IndexT>(proxy);
    bool const in_group = elms > 0;

    vt_debug_print(
      vrt_coll, node,
      "groupConstruction: creating new group: proxy={:x}, elms={}, in_group={}\n",
      proxy, elms, in_group
    );

    theCollection()->createGroupCollection<ColT,IndexT>(proxy, in_group);
  } else {
    /*
     *  If the collection is not immediate (non-static) we need to wait for a
     *  finishedInserting call to build the group.
     */
  }
}


template <typename ColT, typename... Args>
CollectionManager::CollectionProxyWrapType<ColT, typename ColT::IndexType>
CollectionManager::construct(
  typename ColT::IndexType range, Args&&... args
) {
  auto const map_han = getDefaultMap<ColT>();
  return constructMap<ColT,Args...>(range,map_han,args...);
}

template <
  typename ColT, mapping::ActiveMapTypedFnType<typename ColT::IndexType> fn,
  typename... Args
>
CollectionManager::CollectionProxyWrapType<ColT, typename ColT::IndexType>
CollectionManager::construct(
  typename ColT::IndexType range, Args&&... args
) {
  using IndexT = typename ColT::IndexType;
  auto const& map_han = auto_registry::makeAutoHandlerMap<IndexT, fn>();
  return constructMap<ColT,Args...>(range, map_han, args...);
}

template <typename ColT, typename... Args>
CollectionManager::CollectionProxyWrapType<ColT, typename ColT::IndexType>
CollectionManager::constructMap(
  typename ColT::IndexType range, HandlerType const& map_handler,
  Args&&... args
) {
  using IndexT = typename ColT::IndexType;
  using ArgsTupleType = std::tuple<typename std::decay<Args>::type...>;
  using MsgType = CollectionCreateMsg<
    CollectionInfo<ColT, IndexT>, ArgsTupleType, ColT, IndexT
  >;

  auto const& new_proxy = makeNewCollectionProxy();
  auto const& is_static = ColT::isStaticSized();
  auto const& node = theContext()->getNode();
  auto create_msg = makeMessage<MsgType>(
    map_handler, ArgsTupleType{std::forward<Args>(args)...}
  );

  CollectionInfo<ColT, IndexT> info(range, is_static, node, new_proxy);

  if (!is_static) {
    auto const& insert_epoch = theTerm()->makeEpochRootedWave(
      term::SuccessorEpochCapture{no_epoch}
    );
    theTerm()->finishNoActivateEpoch(insert_epoch);
    info.setInsertEpoch(insert_epoch);
    setupNextInsertTrigger<ColT,IndexT>(new_proxy,insert_epoch);
  }

  create_msg->info = info;

  vt_debug_print(
    vrt_coll, node,
    "construct_map: range={}\n", range.toString().c_str()
  );

  theMsg()->broadcastMsg<MsgType,distConstruct<MsgType>>(
    create_msg.get()
  );

  auto create_msg_local = makeMessage<MsgType>(
    map_handler, ArgsTupleType{std::forward<Args>(args)...}
  );
  create_msg_local->info = info;
  CollectionManager::distConstruct<MsgType>(create_msg_local.get());

  return CollectionProxyWrapType<ColT, typename ColT::IndexType>{new_proxy};
}

inline void CollectionManager::insertCollectionInfo(
  VirtualProxyType const& proxy, HandlerType const& map_han,
  EpochType const& insert_epoch
) {
  UniversalIndexHolder<>::insertMap(proxy,map_han,insert_epoch);
}

inline VirtualProxyType CollectionManager::makeNewCollectionProxy() {
  auto const& node = theContext()->getNode();
  return VirtualProxyBuilder::createProxy(details::CurIdent<void>::m_++, node, true, true);
}

/*
 * Support of virtual context collection element dynamic insertion
 */

template <typename ColT, typename IndexT>
/*static*/ void CollectionManager::insertHandler(InsertMsg<ColT,IndexT>* msg) {
  auto const from = theMsg()->getFromNodeCurrentHandler();
  auto const& epoch = msg->epoch_;
  auto const& g_epoch = msg->g_epoch_;
  theCollection()->insert<ColT,IndexT>(
    msg->proxy_,msg->idx_,msg->construct_node_
  );
  theTerm()->consume(epoch,1,from);
  theTerm()->consume(g_epoch,1,from);
}

template <typename ColT, typename IndexT>
/*static*/ void CollectionManager::updateInsertEpochHandler(
  UpdateInsertMsg<ColT,IndexT>* msg
) {
  auto const& untyped_proxy = msg->proxy_.getProxy();
  UniversalIndexHolder<>::insertSetEpoch(untyped_proxy,msg->epoch_);

  /*
   *  Start building the a new group for broadcasts and reductions over the
   *  current set of elements based the distributed snapshot
   */

  auto const elms = theCollection()->groupElementCount<ColT,IndexT>(
    untyped_proxy
  );
  bool const in_group = elms > 0;

  vt_debug_print(
    vrt_coll, node,
    "finishedInsertEpoch: creating new group: elms={}, in_group={}\n",
    elms, in_group
  );

  theCollection()->createGroupCollection<ColT, IndexT>(untyped_proxy, in_group);

  /*
   *  Contribute to reduction for update epoch
   */
  auto const& root = 0;
  auto nmsg = makeMessage<FinishedUpdateMsg>(untyped_proxy);

  using collective::reduce::makeStamp;
  using collective::reduce::StrongEpoch;

  auto stamp = makeStamp<StrongEpoch>(msg->epoch_);
  auto r = theCollection()->reducer();
  r->reduce<FinishedUpdateMsg,finishedUpdateHan>(root, nmsg.get(), stamp);
}

template <typename>
/*static*/ void CollectionManager::finishedUpdateHan(
  FinishedUpdateMsg* msg
) {
  vt_debug_print(
    vrt_coll, node,
    "finishedUpdateHan: proxy={:x}, root={}\n",
    msg->proxy_, msg->isRoot()
  );

  if (msg->isRoot()) {
    /*
     *  Trigger any actions that the user may have registered for when insertion
     *  has fully terminated
     */
    return theCollection()->actInsert<>(msg->proxy_);
  }
}

template <typename ColT, typename IndexT>
void CollectionManager::setupNextInsertTrigger(
  VirtualProxyType const& proxy, EpochType const& insert_epoch
) {
  vt_debug_print(
    vrt_coll, node,
    "setupNextInsertTrigger: proxy={:x}, insert_epoch={}\n",
    proxy, insert_epoch
  );

  auto finished_insert_trigger = [proxy,insert_epoch]{
    vt_debug_print(
      vrt_coll, node,
      "insert finished insert trigger: epoch={}\n",
      insert_epoch
    );
    theCollection()->finishedInsertEpoch<ColT,IndexT>(proxy,insert_epoch);
  };
  auto start_detect = [insert_epoch,finished_insert_trigger]{
    vt_debug_print(
      vrt_coll, node,
      "insert start_detect: epoch={}\n",insert_epoch
    );
    theTerm()->addAction(insert_epoch, finished_insert_trigger);
    theTerm()->activateEpoch(insert_epoch);
  };
  auto iter = insert_finished_action_.find(proxy);
  if (iter == insert_finished_action_.end()) {
    insert_finished_action_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(proxy),
      std::forward_as_tuple(ActionVecType{start_detect})
    );
  } else {
    iter->second.push_back(start_detect);
  }
}

template <typename ColT, typename IndexT>
void CollectionManager::finishedInsertEpoch(
  CollectionProxyWrapType<ColT,IndexT> const& proxy, EpochType const& epoch
) {
  auto const& untyped_proxy = proxy.getProxy();

  vt_debug_print(
    vrt_coll, node,
    "finishedInsertEpoch: (before) proxy={:x}, epoch={}\n",
    untyped_proxy, epoch
  );

  if (not findColHolder<ColT>(untyped_proxy)) {
    return;
  }

  /*
   *  Add trigger for the next insertion phase/epoch finishing
   */
  auto const& next_insert_epoch = theTerm()->makeEpochRootedWave(
    term::SuccessorEpochCapture{no_epoch}
  );
  theTerm()->finishNoActivateEpoch(next_insert_epoch);
  UniversalIndexHolder<>::insertSetEpoch(untyped_proxy,next_insert_epoch);

  auto msg = makeMessage<UpdateInsertMsg<ColT,IndexT>>(
    proxy,next_insert_epoch
  );
  theMsg()->markAsCollectionMessage(msg);
  theMsg()->broadcastMsg<
    UpdateInsertMsg<ColT,IndexT>,updateInsertEpochHandler
  >(msg.get());

  /*
   *  Start building the a new group for broadcasts and reductions over the
   *  current set of elements based the distributed snapshot
   */
  auto const elms = theCollection()->groupElementCount<ColT,IndexT>(
    untyped_proxy
  );
  bool const in_group = elms > 0;

  vt_debug_print(
    vrt_coll, node,
    "finishedInsertEpoch: creating new group: elms={}, in_group={}\n",
    elms, in_group
  );

  theCollection()->createGroupCollection<ColT, IndexT>(untyped_proxy, in_group);

  vt_debug_print(
    vrt_coll, node,
    "finishedInsertEpoch: (after broadcast) proxy={:x}, epoch={}\n",
    untyped_proxy, epoch
  );

  /*
   *  Setup next epoch
   */
  setupNextInsertTrigger<ColT,IndexT>(untyped_proxy,next_insert_epoch);

  vt_debug_print(
    vrt_coll, node,
    "finishedInsertEpoch: (after setup) proxy={:x}, epoch={}\n",
    untyped_proxy, epoch
  );

  /*
   *  Contribute to reduction for update epoch: this forces the update of the
   *  current insertion epoch to be consistent across the managers *before*
   *  triggering the user's finished epoch handler so that all actions on the
   *  corresponding collection after are related to the new insert epoch
   */
  auto const& root = 0;
  auto nmsg = makeMessage<FinishedUpdateMsg>(untyped_proxy);

  using collective::reduce::makeStamp;
  using collective::reduce::StrongEpoch;

  auto stamp = makeStamp<StrongEpoch>(next_insert_epoch);
  auto r = theCollection()->reducer();
  r->reduce<FinishedUpdateMsg,finishedUpdateHan>(root, nmsg.get(), stamp);

  vt_debug_print(
    vrt_coll, node,
    "finishedInsertEpoch: (after reduce) proxy={:x}, epoch={}\n",
    untyped_proxy, epoch
  );
}

template <typename ColT, typename IndexT>
/*static*/ void CollectionManager::actInsertHandler(
  ActInsertMsg<ColT,IndexT>* msg
) {
  auto const& untyped_proxy = msg->proxy_.getProxy();
  return theCollection()->actInsert<>(untyped_proxy);
}

template <typename>
void CollectionManager::actInsert(VirtualProxyType const& proxy) {
  vt_debug_print(
    vrt_coll, node,
    "actInsert: proxy={:x}\n",
    proxy
  );

  auto iter = user_insert_action_.find(proxy);
  if (iter != user_insert_action_.end()) {
    auto action_lst = iter->second;
    user_insert_action_.erase(iter);
    for (auto&& action : action_lst) {
      action();
    }
  }
}

template <typename ColT, typename IndexT>
/*static*/ void CollectionManager::doneInsertHandler(
  DoneInsertMsg<ColT,IndexT>* msg
) {
  auto const& node = msg->action_node_;
  auto const& untyped_proxy = msg->proxy_.getProxy();

  vt_debug_print(
    vrt_coll, node,
    "doneInsertHandler: proxy={:x}, node={}\n",
    untyped_proxy, node
  );

  if (node != uninitialized_destination) {
    auto send = [untyped_proxy,node]{
      auto smsg = makeMessage<ActInsertMsg<ColT,IndexT>>(untyped_proxy);
      theMsg()->markAsCollectionMessage(smsg);
      theMsg()->sendMsg<
        ActInsertMsg<ColT,IndexT>,actInsertHandler<ColT,IndexT>
      >(node,smsg.get());
    };
    return theCollection()->finishedInserting<ColT,IndexT>(msg->proxy_, send);
  } else {
    return theCollection()->finishedInserting<ColT,IndexT>(msg->proxy_);
  }
}

template <typename ColT, typename IndexT>
void CollectionManager::finishedInserting(
  CollectionProxyWrapType<ColT,IndexT> const& proxy,
  ActionType insert_action
) {
  auto const& this_node = theContext()->getNode();
  auto const& untyped_proxy = proxy.getProxy();
  /*
   *  Register the user's action for when insertion is completed across the
   *  whole system, which termination in the insertion epoch can enforce
   */
  if (insert_action) {
    auto iter = user_insert_action_.find(untyped_proxy);
    if (iter ==  user_insert_action_.end()) {
      user_insert_action_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(untyped_proxy),
        std::forward_as_tuple(ActionVecType{insert_action})
      );
    } else {
      iter->second.push_back(insert_action);
    }
  }

  auto const& cons_node = VirtualProxyBuilder::getVirtualNode(untyped_proxy);

  vt_debug_print(
    vrt_coll, node,
    "finishedInserting: proxy={:x}, cons_node={}, this_node={}\n",
    proxy.getProxy(), cons_node, this_node
  );

  if (cons_node == this_node) {
    auto iter = insert_finished_action_.find(untyped_proxy);
    if (iter != insert_finished_action_.end()) {
      auto action_lst = iter->second;
      insert_finished_action_.erase(untyped_proxy);
      for (auto&& action : action_lst) {
        action();
      }
    }
  } else {
    auto node = insert_action ? this_node : uninitialized_destination;
    auto msg = makeMessage<DoneInsertMsg<ColT,IndexT>>(proxy,node);
    theMsg()->markAsCollectionMessage(msg);
    theMsg()->sendMsg<DoneInsertMsg<ColT,IndexT>,doneInsertHandler<ColT,IndexT>>(
      cons_node,msg.get()
    );
  }
}

template <typename ColT, typename IndexT>
NodeType CollectionManager::getMappedNode(
  CollectionProxyWrapType<ColT,IndexT> const& proxy,
  typename ColT::IndexType const& idx
) {
  auto const untyped_proxy = proxy.getProxy();
  auto found_constructed = constructed_.find(untyped_proxy) != constructed_.end();
  if (found_constructed) {
    auto col_holder = findColHolder<ColT,IndexT>(untyped_proxy);
    auto max_idx = col_holder->max_idx;
    auto map_han = UniversalIndexHolder<>::getMap(untyped_proxy);
    bool const& is_functor =
      auto_registry::HandlerManagerType::isHandlerFunctor(map_han);
    auto_registry::AutoActiveMapType fn = nullptr;
    if (is_functor) {
      fn = auto_registry::getAutoHandlerFunctorMap(map_han);
    } else {
      fn = auto_registry::getAutoHandlerMap(map_han);
    }
    auto idx_non_const = idx;
    auto idx_non_const_ptr = &idx_non_const;
    auto const& mapped_node = fn(
      reinterpret_cast<vt::index::BaseIndex*>(idx_non_const_ptr),
      reinterpret_cast<vt::index::BaseIndex*>(&max_idx),
      theContext()->getNumNodes()
    );
    return mapped_node;
  } else {
    return uninitialized_destination;
  }
}

template <typename ColT, typename IndexT>
ColT* CollectionManager::tryGetLocalPtr(
  CollectionProxyWrapType<ColT,IndexT> const& proxy, IndexT idx
) {
  auto const untyped = proxy.getProxy();
  auto elm_holder = findElmHolder<ColT,IndexT>(untyped);

   vtAssert(
     elm_holder != nullptr, "Collection must be registered here"
   );

   auto const& elm_exists = elm_holder->exists(idx);

   if (elm_exists) {
     auto& elm_info = elm_holder->lookup(idx);
     auto elm_ptr = elm_info.getCollection();

     vtAssert(
       elm_ptr != nullptr, "Pointer to the element must not be nullptr"
     );

     return static_cast<ColT*>(elm_ptr);
   } else {
     return nullptr;
   }
}

template <typename ColT, typename IndexT>
void CollectionManager::insert(
  CollectionProxyWrapType<ColT,IndexT> const& proxy, IndexT idx,
  NodeType const& node
) {
  using IdxContextHolder = InsertContextHolder<IndexT>;

  auto const untyped_proxy = proxy.getProxy();
  auto const cur_epoch = theMsg()->getEpoch();
  auto insert_epoch = UniversalIndexHolder<>::insertGetEpoch(untyped_proxy);
  vtAssert(insert_epoch != no_epoch, "Epoch should be valid");

  vt_debug_print(
    vrt_coll, node,
    "insert: proxy={:x}\n",
    untyped_proxy
  );

  theTerm()->produce(insert_epoch);

  bufferOpOrExecute<ColT>(
    untyped_proxy,
    BufferTypeEnum::Broadcast,
    static_cast<BufferReleaseEnum>(AfterFullyConstructed | AfterMetaDataKnown),
    cur_epoch,
    [=]() -> messaging::PendingSend {
      auto map_han = UniversalIndexHolder<>::getMap(untyped_proxy);
      auto const max_idx = getRange<ColT>(untyped_proxy);
      auto const mapped_node = getMapped<ColT>(map_han, idx, max_idx);
      auto const has_explicit_node = node != uninitialized_destination;
      auto const insert_node = has_explicit_node ? node : mapped_node;
      auto const this_node = theContext()->getNode();
      auto cur_idx = idx;

      if (insert_node == this_node) {
        auto const& num_elms = max_idx.getSize();
        std::tuple<> tup;

        // Set the current context index to `idx`, enabled the user to query the
        // index during the constructor
        IdxContextHolder::set(&cur_idx,untyped_proxy);

#       if vt_check_enabled(detector) && vt_check_enabled(cons_multi_idx)
        auto new_vc = DerefCons::derefTuple<ColT,IndexT,std::tuple<>>(
          num_elms, cur_idx, &tup
        );
#       else
        auto new_vc = CollectionManager::runConstructor<ColT, IndexT>(
          num_elms, cur_idx, &tup, std::make_index_sequence<0>{}
        );
#       endif

        /*
         * Set direct attributes of the newly constructed element directly on
         * the user's class
         */
        CollectionTypeAttorney::setup(new_vc, num_elms, cur_idx, untyped_proxy);

        theCollection()->insertCollectionElement<ColT, IndexT>(
          std::move(new_vc), cur_idx, max_idx, map_han, untyped_proxy, false,
          mapped_node
        );

        // Clear the current index context
        IdxContextHolder::clear();
      } else {
        auto msg = makeMessage<InsertMsg<ColT,IndexT>>(
          proxy,max_idx,idx,insert_node,mapped_node,insert_epoch,cur_epoch
        );
        theTerm()->produce(insert_epoch,1,insert_node);
        theTerm()->produce(cur_epoch,1,insert_node);
        theMsg()->markAsCollectionMessage(msg);
        theMsg()->sendMsg<InsertMsg<ColT,IndexT>,insertHandler<ColT,IndexT>>(
          insert_node,msg.get()
        );
      }

      theTerm()->consume(insert_epoch);
      return messaging::PendingSend{nullptr};
    }
  );
}

/*
 * Support of virtual context collection element migration
 */

template <typename ColT>
MigrateStatus CollectionManager::migrate(
  VrtElmProxy<ColT, typename ColT::IndexType> proxy, NodeType const& dest
) {
  using IndexT = typename ColT::IndexType;
  auto const col_proxy = proxy.getCollectionProxy();
  auto const elm_proxy = proxy.getElementProxy();
  auto const idx = elm_proxy.getIndex();

  auto const epoch = theMsg()->getEpoch();
  theTerm()->produce(epoch);
  schedule([=]{
    theMsg()->pushEpoch(epoch);
    migrateOut<ColT,IndexT>(col_proxy, idx, dest);
    theMsg()->popEpoch(epoch);
    theTerm()->consume(epoch);
  });

  return MigrateStatus::PendingLocalAction;
}

template <typename ColT, typename IndexT>
MigrateStatus CollectionManager::migrateOut(
  VirtualProxyType const& col_proxy, IndexT const& idx, NodeType const& dest
) {
 auto const& this_node = theContext()->getNode();

 vt_debug_print(
   vrt_coll, node,
   "migrateOut: col_proxy={:x}, this_node={}, dest={}, "
   "idx={}\n",
   col_proxy, this_node, dest, print_index(idx)
 );

 if (this_node != dest) {
   auto const& proxy = CollectionProxy<ColT, IndexT>(col_proxy).operator()(
     idx
   );
   auto elm_holder = findElmHolder<ColT, IndexT>(col_proxy);
   vtAssert(
     elm_holder != nullptr, "Element must be registered here"
   );

   #if vt_check_enabled(runtime_checks)
   {
     bool const exists = elm_holder->exists(idx);
     vtAssert(
       exists, "Local element must exist here for migration to occur"
     );
   }
   #endif

   bool const exists = elm_holder->exists(idx);
   if (!exists) {
     return MigrateStatus::ElementNotLocal;
   }

   vt_debug_print(
     vrt_coll, node,
     "migrateOut: (before remove) holder numElements={}\n",
     elm_holder->numElements()
   );

   auto& coll_elm_info = elm_holder->lookup(idx);
   auto map_fn = coll_elm_info.map_fn;
   auto range = coll_elm_info.max_idx;
   auto col_unique_ptr = elm_holder->remove(idx);
   auto& typed_col_ref = *static_cast<ColT*>(col_unique_ptr.get());

   vt_debug_print(
     vrt_coll, node,
     "migrateOut: (after remove) holder numElements={}\n",
     elm_holder->numElements()
   );

   /*
    * Invoke the virtual prelude migrate out function
    */
   col_unique_ptr->preMigrateOut();

   vt_debug_print(
     vrt_coll, node,
     "migrateOut: col_proxy={:x}, idx={}, dest={}: serializing collection elm\n",
     col_proxy, print_index(idx), dest
   );

   using MigrateMsgType = MigrateMsg<ColT, IndexT>;

   auto msg = makeMessage<MigrateMsgType>(
     proxy, this_node, dest, map_fn, range, &typed_col_ref
   );

   theMsg()->sendMsg<
     MigrateMsgType, MigrateHandlers::migrateInHandler<ColT, IndexT>
   >(dest, msg.get());

   theLocMan()->getCollectionLM<ColT, IndexT>(col_proxy)->entityMigrated(
     proxy, dest
   );

   /*
    * Invoke the virtual epilog migrate out function
    */
   col_unique_ptr->epiMigrateOut();

   vt_debug_print(
     vrt_coll, node,
     "migrateOut: col_proxy={:x}, idx={}, dest={}: invoking destroy()\n",
     col_proxy, print_index(idx), dest
   );

   /*
    * Invoke the virtual destroy function and then null std::unique_ptr<ColT>,
    * which should cause the destructor to fire
    */
   col_unique_ptr->destroy();
   col_unique_ptr = nullptr;

   elm_holder->applyListeners(
     listener::ElementEventEnum::ElementMigratedOut, idx
   );

   return MigrateStatus::MigratedToRemote;
 } else {
   #if vt_check_enabled(runtime_checks)
     vtAssert(
       false, "Migration should only be called when to_node is != this_node"
     );
   #else
     // Do nothing
   #endif
   return MigrateStatus::NoMigrationNecessary;
 }
}

template <typename ColT, typename IndexT>
MigrateStatus CollectionManager::migrateIn(
  VirtualProxyType const& proxy, IndexT const& idx, NodeType const& from,
  VirtualPtrType<ColT, IndexT> vrt_elm_ptr, IndexT const& max,
  HandlerType const& map_han
) {
  vt_debug_print(
    vrt_coll, node,
    "CollectionManager::migrateIn: proxy={:x}, idx={}, from={}, ptr={}\n",
    proxy, print_index(idx), from, print_ptr(vrt_elm_ptr.get())
  );

  auto vc_raw_ptr = vrt_elm_ptr.get();

  /*
   * Invoke the virtual prelude migrate-in function
   */
  vc_raw_ptr->preMigrateIn();

  CollectionProxy<ColT, IndexT>(proxy).operator()(idx);

  // Always assign a new temp element ID for LB statistic tracking
  vrt_elm_ptr->temp_elm_id_ = theNodeStats()->getNextElm();

  bool const is_static = ColT::isStaticSized();

  auto idx_copy = idx;
  auto max_idx_copy = max;
  auto const cur_cast = static_cast<vt::index::BaseIndex*>(&idx_copy);
  auto const max_cast = static_cast<vt::index::BaseIndex*>(&max_idx_copy);
  auto fn = auto_registry::getHandlerMap(map_han);
  auto const home_node = fn(cur_cast, max_cast, theContext()->getNumNodes());

  auto const inserted = insertCollectionElement<ColT, IndexT>(
    std::move(vrt_elm_ptr), idx, max, map_han, proxy, is_static,
    home_node, true, from
  );

  /*
   * Invoke the virtual epilog migrate-in function
   */
  vc_raw_ptr->epiMigrateIn();

  if (inserted) {
    return MigrateStatus::MigrateInLocal;
  } else {
    return MigrateStatus::DestroyedDuringMigrated;
  }
}

template <typename ColT, typename IndexT>
void CollectionManager::destroy(
  CollectionProxyWrapType<ColT,IndexT> const& proxy
) {
  using DestroyMsgType = DestroyMsg<ColT, IndexT>;
  auto const& this_node = theContext()->getNode();
  auto msg = makeMessage<DestroyMsgType>(proxy, this_node);
  theMsg()->markAsCollectionMessage(msg);
  theMsg()->broadcastMsg<DestroyMsgType, DestroyHandlers::destroyNow>(msg.get());
  DestroyHandlers::destroyNow(msg.get());
}

template <typename ColT, typename IndexT>
void CollectionManager::incomingDestroy(
  CollectionProxyWrapType<ColT,IndexT> const& proxy
) {
  auto iter = cleanup_fns_.find(proxy.getProxy());
  if (iter != cleanup_fns_.end()) {
    auto fns = std::move(iter->second);
    cleanup_fns_.erase(iter);
    for (auto fn : fns) {
      fn();
    }
  }
}

template <typename ColT, typename IndexT>
void CollectionManager::destroyMatching(
  CollectionProxyWrapType<ColT,IndexT> const& proxy
) {
  vt_debug_print(
    vrt_coll, node,
    "destroyMatching: proxy={:x}\n",
    proxy.getProxy()
  );

  auto const untyped_proxy = proxy.getProxy();
  UniversalIndexHolder<>::destroyCollection(untyped_proxy);
  auto elm_holder = findElmHolder<ColT,IndexT>(untyped_proxy);
  if (elm_holder) {
    elm_holder->foreach([&](IndexT const& idx, CollectionBase<ColT,IndexT>*) {
      elm_holder->applyListeners(
        listener::ElementEventEnum::ElementDestroyed, idx
      );
    });
    elm_holder->destroyAll();
  }

  auto const is_static = ColT::isStaticSized();
  if (not is_static) {
    auto const& this_node = theContext()->getNode();
    auto const cons_node = VirtualProxyBuilder::getVirtualNode(untyped_proxy);
    if (cons_node == this_node) {
      finishedInserting(proxy, nullptr);
    }
  }

  EntireHolder<ColT, IndexT>::remove(untyped_proxy);

  auto iter = cleanup_fns_.find(untyped_proxy);
  if (iter != cleanup_fns_.end()) {
    cleanup_fns_.erase(iter);
  }
}

template <typename ColT, typename IndexT>
CollectionHolder<ColT, IndexT>* CollectionManager::findColHolder(
  VirtualProxyType const& proxy
) {
  #pragma sst global proxy_container_
  auto& holder_container = EntireHolder<ColT, IndexT>::proxy_container_;
  auto holder_iter = holder_container.find(proxy);
  auto const& found_holder = holder_iter != holder_container.end();
  if (found_holder) {
    return holder_iter->second.get();
  } else {
    return nullptr;
  }
}

template <typename ColT, typename IndexT>
Holder<ColT, IndexT>* CollectionManager::findElmHolder(
  VirtualProxyType const& proxy
) {
  auto ret = findColHolder<ColT, IndexT>(proxy);
  if (ret != nullptr) {
    return &ret->holder_;
  } else {
    return nullptr;
  }
}

template <typename ColT, typename IndexT>
Holder<ColT, IndexT>* CollectionManager::findElmHolder(
  CollectionProxyWrapType<ColT> proxy
) {
  return findElmHolder<ColT,IndexT>(proxy.getProxy());
}

template <typename>
std::size_t CollectionManager::numCollections() {
  return UniversalIndexHolder<>::getNumCollections();
}

template <typename>
std::size_t CollectionManager::numReadyCollections() {
  return UniversalIndexHolder<>::getNumReadyCollections();
}

template <typename>
void CollectionManager::resetReadyPhase() {
  UniversalIndexHolder<>::resetPhase();
}

template <typename>
bool CollectionManager::readyNextPhase() {
  auto const ready = UniversalIndexHolder<>::readyNextPhase();
  return ready;
}

template <typename>
void CollectionManager::makeCollectionReady(VirtualProxyType const proxy) {
  UniversalIndexHolder<>::makeCollectionReady(proxy);
}

template <typename ColT>
void CollectionManager::elmFinishedLB(
  VirtualElmProxyType<ColT> const& proxy, PhaseType phase
) {
  auto const& col_proxy = proxy.getCollectionProxy();
  auto const& idx = proxy.getElementProxy().getIndex();
  auto elm_holder = findElmHolder<ColT>(col_proxy);
  vtAssertInfo(
    elm_holder != nullptr, "Must find element holder at elmFinishedLB",
    col_proxy, phase
  );
  elm_holder->runLBCont(idx);
}

template <
  typename MsgT, typename ColT, ActiveColMemberTypedFnType<MsgT,ColT> f
>
void CollectionManager::elmReadyLB(
  VirtualElmProxyType<ColT> const& proxy, PhaseType phase, MsgT* msg,
  bool do_sync
) {
  auto lb_han = auto_registry::makeAutoHandlerCollectionMem<ColT,MsgT,f>(msg);
  auto pmsg = promoteMsg(msg);

#if !vt_check_enabled(lblite)
  theCollection()->sendMsgUntypedHandler<MsgT>(proxy,pmsg.get(),lb_han,true);
  return;
#endif

  auto const col_proxy = proxy.getCollectionProxy();
  auto const idx = proxy.getElementProxy().getIndex();
  auto elm_holder = findElmHolder<ColT>(col_proxy);
  vtAssertInfo(
    elm_holder != nullptr, "Must find element holder at elmReadyLB",
    col_proxy, phase
  );

  auto const cur_epoch = theMsg()->getEpochContextMsg(msg);

  vt_debug_print(
    lb, node,
    "elmReadyLB: proxy={:x}, idx={}, phase={}, msg={}, epoch={:x}\n",
    col_proxy, idx, phase, pmsg, cur_epoch
  );

  theTerm()->produce(cur_epoch);
  elm_holder->addLBCont(idx,[pmsg,proxy,lb_han,cur_epoch]{
    theCollection()->sendMsgUntypedHandler<MsgT>(proxy,pmsg.get(),lb_han,true);
    theTerm()->consume(cur_epoch);
  });

  auto iter = release_lb_.find(col_proxy);
  if (iter == release_lb_.end()) {
    release_lb_[col_proxy] = [this,col_proxy]{
      auto cur_elm_holder = findElmHolder<ColT>(col_proxy);
      cur_elm_holder->runLBCont();
    };
  }

  elmReadyLB<ColT>(proxy,phase,do_sync,nullptr);
}

template <typename ColT>
void CollectionManager::elmReadyLB(
  VirtualElmProxyType<ColT> const& proxy, PhaseType in_phase,
  bool do_sync, ActionFinishedLBType cont
) {

  vt_debug_print(
    lb, node,
    "elmReadyLB: index={} ready at sync={}, phase={}\n",
    proxy.getElementProxy().getIndex(), do_sync, in_phase
  );

#if !vt_check_enabled(lblite)
  cont();
  return;
#endif

  auto const col_proxy = proxy.getCollectionProxy();
  auto const idx = proxy.getElementProxy().getIndex();
  auto elm_holder = findElmHolder<ColT>(col_proxy);

  PhaseType phase = in_phase;
  if (phase == no_lb_phase) {
    bool const elm_exists = elm_holder->exists(idx);
    if (!(elm_exists)) fmt::print("Element must exist idx={}\n", idx);
    auto& holder = elm_holder->lookup(idx);
    auto elm = holder.getCollection();
    if (!(elm != nullptr)) fmt::print("Must have valid element");
    phase = elm->stats_.getPhase();
  }

  vtAssertInfo(
    elm_holder != nullptr, "Must find element holder at elmReadyLB",
    col_proxy, phase
  );

  vt_debug_print(
    lb, node,
    "elmReadyLB: proxy={:x}, idx={} ready at phase={}\n",
    col_proxy, idx, phase
  );

  if (cont != nullptr) {
    theTerm()->produce(term::any_epoch_sentinel);
    lb_continuations_.push_back(cont);
  }
  if (elm_holder) {
    vt_debug_print(lb, node, "has elm_holder: exists={}\n", elm_holder->exists(idx));

    vtAssert(
      elm_holder->exists(idx),
      "Collection element must be local and currently reside on this node"
    );
    elm_holder->addReady();
    auto const num_ready = elm_holder->numReady();
    auto const num_total = elm_holder->numElements();

    vt_debug_print(
      lb, node,
      "elmReadyLB: proxy={:x}, ready={}, total={} at phase={}\n",
      col_proxy, num_ready, num_total, phase
    );

    if (num_ready == num_total) {
      elm_holder->clearReady();

      vt_debug_print(
        lb, node,
        "elmReadyLB: all local elements of proxy={:x} ready at phase={}\n",
        col_proxy, phase
      );
    }

    using namespace balance;
    CollectionProxyWrapType<ColT> cur_proxy(col_proxy);
    using MsgType = PhaseMsg<ColT>;
    auto msg = makeMessage<MsgType>(phase, cur_proxy, do_sync, false);

#if vt_check_enabled(lblite)
    msg->setLBLiteInstrument(false);
#endif

    vt_debug_print(
      lb, node,
      "elmReadyLB: invoking syncNextPhase on  proxy={:x}, at phase={}\n",
      col_proxy, phase
    );

    theCollection()->sendMsg<MsgType,ElementStats::syncNextPhase<ColT>>(
      cur_proxy[idx], msg.get()
    );
  }
}

template <typename ColT>
void CollectionManager::nextPhase(
  CollectionProxyWrapType<ColT, typename ColT::IndexType> const& proxy,
  PhaseType const& cur_phase, ActionFinishedLBType continuation
) {
  using namespace balance;
  using MsgType = PhaseMsg<ColT>;
  auto msg = makeMessage<MsgType>(cur_phase, proxy, true, false);
  auto const instrument = false;

  vt_debug_print(
    vrt_coll, node,
    "nextPhase: broadcasting: cur_phase={}\n",
    cur_phase
  );

  if (continuation != nullptr) {
    theTerm()->produce(term::any_epoch_sentinel);
    lb_continuations_.push_back(continuation);

    auto const& untyped_proxy = proxy.getProxy();
    auto iter = lb_no_elm_.find(untyped_proxy);
    if (iter ==lb_no_elm_.end()) {
      lb_no_elm_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(untyped_proxy),
        std::forward_as_tuple([this,untyped_proxy]{
          auto elm_holder =
            findElmHolder<ColT,typename ColT::IndexType>(untyped_proxy);
          auto const& num_elms = elm_holder->numElements();
          // this collection manager does not participate in reduction
          if (num_elms == 0) {
            /*
             * @todo: Implement child elision in reduction tree and up
             * propagation
             */
          }
        })
      );
    }
  }

  #if vt_check_enabled(lblite)
  msg->setLBLiteInstrument(instrument);
  vt_debug_print(
  vrt_coll, node,
    "nextPhase: broadcasting: instrument={}, cur_phase={}\n",
    msg->lbLiteInstrument(), cur_phase
    );
  #endif

  theCollection()->broadcastMsg<MsgType,ElementStats::syncNextPhase<ColT>>(
    proxy, msg.get(), instrument
  );
}

template <typename always_void>
void CollectionManager::checkReduceNoElements() {
  // @todo
}

template <typename always_void>
/*static*/ void CollectionManager::releaseLBPhase(CollectionPhaseMsg* msg) {
  theCollection()->releaseLBContinuation();
}

template <typename>
void CollectionManager::releaseLBContinuation() {
  vt_debug_print(
    lb, node,
    "releaseLBContinuation\n"
  );
  UniversalIndexHolder<>::resetPhase();
  if (lb_continuations_.size() > 0) {
    auto continuations = lb_continuations_;
    lb_continuations_.clear();
    for (auto&& elm : continuations) {
      theTerm()->consume(term::any_epoch_sentinel);
      elm();
    }
  }
  for (auto&& elm : release_lb_) {
    elm.second();
  }
  release_lb_.clear();
}

template <typename MsgT, typename ColT>
CollectionManager::DispatchHandlerType
CollectionManager::getDispatchHandler() {
  auto const idx = makeVrtDispatch<MsgT,ColT>();
  return idx;
}

template <typename always_void>
DispatchBasePtrType
CollectionManager::getDispatcher(DispatchHandlerType const& han) {
  return getDispatch(han);
}

template <typename ColT, typename IndexT>
int CollectionManager::registerElementListener(
  VirtualProxyType proxy, listener::ListenFnType<IndexT> fn
) {
  auto elm_holder = findElmHolder<ColT>(proxy);
  return elm_holder->addListener(fn);
}

template <typename ColT, typename IndexT>
void CollectionManager::unregisterElementListener(
  VirtualProxyType proxy, int element
) {
  auto elm_holder = findElmHolder<ColT>(proxy);
  elm_holder->removeListener(element);
}

template <typename ColT, typename IndexT>
IndexT CollectionManager::getRange(VirtualProxyType proxy) {
  auto col_holder = findColHolder<ColT>(proxy);
  return col_holder->max_idx;
}

template <typename IndexT>
std::string CollectionManager::makeMetaFilename(
  std::string file_base, bool make_sub_dirs
) {
  auto this_node = theContext()->getNode();
  if (make_sub_dirs) {

    auto subdir = fmt::format("{}/directory-{}", file_base, this_node);
    int flag = mkdir(subdir.c_str(), S_IRWXU);
    if (flag < 0 && errno != EEXIST) {
      throw std::runtime_error("Failed to create directory: " + subdir);
    }

    return fmt::format(
      "{}/directory-{}/{}.directory", file_base, this_node, this_node
    );
  } else {
    return fmt::format("{}.{}.directory", file_base, this_node);
  }
}

template <typename IndexT>
std::string CollectionManager::makeFilename(
  IndexT range, IndexT idx, std::string file_base, bool make_sub_dirs,
  int files_per_directory
) {
  vtAssert(files_per_directory >= 1, "Must be >= 1");

  std::string idx_str = "";
  for (int i = 0; i < idx.ndims(); i++) {
    idx_str += fmt::format("{}{}", idx[i], i < idx.ndims() - 1 ? "." : "");
  }
  if (make_sub_dirs) {
    auto lin = mapping::linearizeDenseIndexColMajor(&idx, &range);
    auto dir_name = lin / files_per_directory;

    int flag = 0;
    flag = mkdir(file_base.c_str(), S_IRWXU);
    if (flag < 0 && errno != EEXIST) {
      throw std::runtime_error("Failed to create directory: " + file_base);
    }

    auto subdir = fmt::format("{}/{}", file_base, dir_name);
    flag = mkdir(subdir.c_str(), S_IRWXU);
    if (flag < 0 && errno != EEXIST) {
      throw std::runtime_error("Failed to create directory: " + subdir);
    }

    return fmt::format("{}/{}/{}", file_base, dir_name, idx_str);
  } else {
    return fmt::format("{}-{}", file_base, idx_str);
  }
}

template <typename ColT, typename IndexT>
void CollectionManager::checkpointToFile(
  CollectionProxyWrapType<ColT> proxy, std::string const& file_base,
  bool make_sub_dirs, int files_per_directory
) {
  auto proxy_bits = proxy.getProxy();

  vt_debug_print(
    vrt_coll, node,
    "checkpointToFile: proxy={:x}, file_base={}\n",
    proxy_bits, file_base
  );

  // Get the element holder
  auto holder_ = findElmHolder<ColT>(proxy_bits);
  vtAssert(holder_ != nullptr, "Must have valid holder for collection");

  auto range = getRange<ColT>(proxy_bits);

  CollectionDirectory<IndexT> directory;

  holder_->foreach([&](IndexT const& idx, CollectionBase<ColT,IndexT>* elm) {
    auto const name = makeFilename(
      range, idx, file_base, make_sub_dirs, files_per_directory
    );
    auto const bytes = checkpoint::getSize(*static_cast<ColT*>(elm));
    directory.elements_.emplace_back(
      typename CollectionDirectory<IndexT>::Element{idx, name, bytes}
    );

    checkpoint::serializeToFile(*static_cast<ColT*>(elm), name);
  });

  auto const directory_name = makeMetaFilename<IndexT>(file_base, make_sub_dirs);
  checkpoint::serializeToFile(directory, directory_name);
}

template <typename ColT>
CollectionManager::CollectionProxyWrapType<ColT>
CollectionManager::restoreFromFile(
  typename ColT::IndexType range, std::string const& file_base
) {
  using IndexType = typename ColT::IndexType;
  using DirectoryType = CollectionDirectory<IndexType>;

  auto token = constructInsert<ColT>(range);

  auto metadata_file_name = makeMetaFilename<IndexType>(file_base, false);

  if (access(metadata_file_name.c_str(), F_OK) == -1) {
    // file doesn't exist, try looking in sub-directory
    metadata_file_name = makeMetaFilename<IndexType>(file_base, true);
  }

  if (access(metadata_file_name.c_str(), F_OK) == -1) {
    throw std::runtime_error("Collection directory file cannot be found");
  }

  auto directory = checkpoint::deserializeFromFile<DirectoryType>(
    metadata_file_name
  );

  for (auto&& elm : directory->elements_) {
    auto idx = elm.idx_;
    auto file_name = elm.file_name_;

    if (access(file_name.c_str(), F_OK) == -1) {
      auto err = fmt::format(
        "Collection element file cannot be found: idx={}, file={}",
        idx, file_name
      );
      throw std::runtime_error(err);
    }

    // @todo: error check the file read with bytes in directory

    auto col_ptr = checkpoint::deserializeFromFile<ColT>(file_name);
    token[idx].insertPtr(std::move(col_ptr));
  }

  return finishedInsert(std::move(token));
}

inline bool CollectionManager::checkReady(
  VirtualProxyType proxy, BufferReleaseEnum release
) {
  return getReadyBits(proxy, release) == release;
}

inline BufferReleaseEnum CollectionManager::getReadyBits(
  VirtualProxyType proxy, BufferReleaseEnum release
) {
  auto release_state = getState(proxy);
  auto ret = static_cast<BufferReleaseEnum>(release & release_state);

  vt_debug_print(
    vrt_coll, node,
    "getReadyBits: proxy={:x}, check release={:b}, state={:b}, ret={:b}\n",
    proxy, release, release_state, ret
  );

  return ret;
}

template <typename ColT>
messaging::PendingSend CollectionManager::bufferOp(
  VirtualProxyType proxy, BufferTypeEnum type, BufferReleaseEnum release,
  EpochType epoch, ActionPendingType action
) {
  vtAssertInfo(
    !checkReady(proxy, release), "Should not be ready if buffering",
    proxy
  );
  theTerm()->produce(epoch);
  buffers_[proxy][type][release].push_back([=]() -> messaging::PendingSend {
    theMsg()->pushEpoch(epoch);
    auto ps = action();
    theMsg()->popEpoch(epoch);
    theTerm()->consume(epoch);
    return ps;
  });
  return messaging::PendingSend{nullptr};
}

template <typename ColT>
messaging::PendingSend CollectionManager::bufferOpOrExecute(
  VirtualProxyType proxy, BufferTypeEnum type, BufferReleaseEnum release,
  EpochType epoch, ActionPendingType action
) {
  if (checkReady(proxy, release)) {
    theMsg()->pushEpoch(epoch);
    auto ps = action();
    theMsg()->popEpoch(epoch);
    return ps;
  } else {
    return bufferOp<ColT>(proxy, type, release, epoch, action);
  }
}

inline void CollectionManager::triggerReadyOps(
  VirtualProxyType proxy, BufferTypeEnum type
) {
  auto proxy_iter = buffers_.find(proxy);
  if (proxy_iter != buffers_.end()) {
    auto type_iter = proxy_iter->second.find(type);
    if (type_iter != proxy_iter->second.end()) {
      auto release_map = type_iter->second;
      for (auto iter = release_map.begin(); iter != release_map.end(); ) {
        if (checkReady(proxy, iter->first)) {
          for (auto&& action : iter->second) {
            action();
          }
          iter = release_map.erase(iter);
        } else {
          iter++;
        }
      }
    }
  }
}

template <typename MsgT>
messaging::PendingSend CollectionManager::schedule(
  MsgT msg, bool execute_now, EpochType cur_epoch, ActionType action
) {
  theTerm()->produce(cur_epoch);
  return messaging::PendingSend(msg, [=](MsgVirtualPtr<BaseMsgType> inner_msg){
    auto fn = [=]{
      theMsg()->pushEpoch(cur_epoch);
      action();
      theMsg()->popEpoch(cur_epoch);
      theTerm()->consume(cur_epoch);
    };
    if (execute_now) {
      fn();
    } else {
      schedule(fn);
    }
  });
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_IMPL_H*/
