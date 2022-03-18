/*
//@HEADER
// *****************************************************************************
//
//                                manager.impl.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_MANAGER_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_MANAGER_IMPL_H

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
#include "vt/vrt/collection/migrate/migrate_msg.h"
#include "vt/vrt/collection/migrate/migrate_handlers.h"
#include "vt/vrt/collection/active/active_funcs.h"
#include "vt/vrt/collection/destroy/destroy_msg.h"
#include "vt/vrt/collection/destroy/destroy_handlers.h"
#include "vt/vrt/collection/balance/phase_msg.h"
#include "vt/vrt/collection/dispatch/dispatch.h"
#include "vt/vrt/collection/dispatch/registry.h"
#include "vt/vrt/collection/holders/collection_context_holder.h"
#include "vt/vrt/collection/collection_directory.h"
#include "vt/vrt/collection/balance/node_stats.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/registry/auto/map/auto_registry_map.h"
#include "vt/registry/auto/collection/auto_registry_collection.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/topos/mapping/mapping_headers.h"
#include "vt/termination/term_headers.h"
#include "vt/serialization/sizer.h"
#include "vt/group/group_headers.h"
#include "vt/pipe/pipe_headers.h"
#include "vt/scheduler/scheduler.h"
#include "vt/phase/phase_manager.h"
#include "vt/runnable/invoke.h"
#include "vt/runnable/make_runnable.h"

#include <tuple>
#include <utility>
#include <functional>
#include <cassert>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

#include <fmt/core.h>
#include <fmt/ostream.h>

namespace vt { namespace vrt { namespace collection {

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
  typeless_holder_.destroyAllLive();
}

template <typename ColT, typename IndexT, typename... Args>
/*static*/ typename CollectionManager::VirtualPtrType<IndexT>
CollectionManager::runConstructor(Args&&... args) {
  return std::make_unique<ColT>(std::forward<Args>(args)...);
}

template <typename ColT>
void CollectionManager::addCleanupFn(VirtualProxyType proxy) {
  cleanup_fns_[proxy].push_back([=]{
    CollectionProxyWrapType<ColT> typed_proxy(proxy);
    destroyMatching(typed_proxy);
  });
}

template <typename ColT>
std::size_t CollectionManager::groupElementCount(VirtualProxyType const& p) {
  auto elm_holder = theCollection()->findElmHolder<typename ColT::IndexType>(p);
  std::size_t const num_elms = elm_holder->numElements();

  vt_debug_print(
    verbose, vrt_coll,
    "groupElementcount: num_elms={}, proxy={:x}, proxy={:x}\n",
    num_elms, p, p
  );

  return num_elms;
}

template <typename ColT>
GroupType CollectionManager::createGroupCollection(
  VirtualProxyType const& proxy, bool const in_group
) {
  using IndexType = typename ColT::IndexType;

  vt_debug_print(
    terse, vrt_coll,
    "createGroupCollection: proxy={:x}, in_group={}\n",
    proxy, in_group
  );

  {
    // Delete the old group that is no longer needed since we are replacing it
    auto elm_holder = theCollection()->findElmHolder<IndexType>(proxy);
    auto old_group_id = elm_holder->group();
    if (old_group_id != no_group and old_group_id != default_group) {
      auto old_group_default = theGroup()->isGroupDefault(old_group_id);
      if (not old_group_default) {
        theGroup()->deleteGroupCollective(old_group_id);
      }
    }
  }

  auto const group_id = theGroup()->newGroupCollective(
    in_group, [proxy](GroupType new_group){
      auto const& group_root = theGroup()->groupRoot(new_group);
      auto const& is_group_default = theGroup()->isGroupDefault(new_group);
      auto elm_holder = theCollection()->findElmHolder<IndexType>(proxy);
      elm_holder->setGroup(new_group);
      elm_holder->setUseGroup(!is_group_default);
      elm_holder->setGroupReady(true);
      if (!is_group_default) {
        elm_holder->setGroupRoot(group_root);
      }

      vt_debug_print(
        normal, vrt_coll,
        "group finished construction: proxy={:x}, new_group={:x}, use_group={}, "
        "ready={}, root={}, is_group_default={}\n",
        proxy, new_group, elm_holder->useGroup(), elm_holder->groupReady(),
        group_root, is_group_default
      );
    }
  );

  vt_debug_print(
    verbose, vrt_coll,
    "createGroupCollection (after): proxy={:x}, in_group={}, group_id={:x}\n",
    proxy, in_group, group_id
  );

  return group_id;
}

template <typename ColT, typename IndexT, typename MsgT, typename UserMsgT>
/*static*/ CollectionManager::IsWrapType<ColT, UserMsgT, MsgT>
CollectionManager::collectionAutoMsgDeliver(
  MsgT* msg, Indexable<IndexT>* base, HandlerType han, NodeType from,
  trace::TraceEventIDType event, bool immediate
) {
  auto user_msg = makeMessage<UserMsgT>(std::move(msg->getMsg()));

  // Expand out the index for tracing purposes; Projections takes up to
  // 4-dimensions
  auto idx = base->getIndex();
  uint64_t const idx1 = idx.ndims() > 0 ? idx[0] : 0;
  uint64_t const idx2 = idx.ndims() > 1 ? idx[1] : 0;
  uint64_t const idx3 = idx.ndims() > 2 ? idx[2] : 0;
  uint64_t const idx4 = idx.ndims() > 3 ? idx[3] : 0;

  runnable::makeRunnable(user_msg, true, han, from)
    .withTDEpoch(theMsg()->getEpochContextMsg(msg))
    .withCollection(base)
    .withTraceIndex(event, idx1, idx2, idx3, idx4)
    .withLBStats(base, msg)
    .runOrEnqueue(immediate);
}

template <typename ColT, typename IndexT, typename MsgT, typename UserMsgT>
/*static*/ CollectionManager::IsNotWrapType<ColT, UserMsgT, MsgT>
CollectionManager::collectionAutoMsgDeliver(
  MsgT* msg, Indexable<IndexT>* base, HandlerType han, NodeType from,
  trace::TraceEventIDType event, bool immediate
) {
  // Expand out the index for tracing purposes; Projections takes up to
  // 4-dimensions
  auto idx = base->getIndex();
  uint64_t const idx1 = idx.ndims() > 0 ? idx[0] : 0;
  uint64_t const idx2 = idx.ndims() > 1 ? idx[1] : 0;
  uint64_t const idx3 = idx.ndims() > 2 ? idx[2] : 0;
  uint64_t const idx4 = idx.ndims() > 3 ? idx[3] : 0;

  auto m = promoteMsg(msg);
  runnable::makeRunnable(m, true, han, from)
    .withTDEpoch(theMsg()->getEpochContextMsg(msg))
    .withCollection(base)
    .withTraceIndex(event, idx1, idx2, idx3, idx4)
    .withLBStats(base)
    .runOrEnqueue(immediate);
}

template <typename ColT, typename IndexT, typename MsgT>
/*static*/ void CollectionManager::collectionBcastHandler(MsgT* msg) {
  auto const col_msg = static_cast<CollectionMessage<ColT>*>(msg);
  auto const bcast_proxy = col_msg->getBcastProxy();
  auto const& group = envelopeGetGroup(msg->env);
  auto const& cur_epoch = theMsg()->getEpoch();
  auto const& msg_epoch = envelopeGetEpoch(msg->env);
  theMsg()->pushEpoch(cur_epoch);
  vt_debug_print(
    terse, vrt_coll,
    "collectionBcastHandler: bcast_proxy={:x}, han={}, "
    "epoch={:x}, msg epoch={:x}, group={}, default group={}\n",
    bcast_proxy, col_msg->getVrtHandler(),
    cur_epoch, msg_epoch, group, default_group
  );
  auto elm_holder = theCollection()->findElmHolder<IndexT>(bcast_proxy);
  if (elm_holder) {
    auto const handler = col_msg->getVrtHandler();
    vt_debug_print(
      normal, vrt_coll,
      "broadcast apply: size={}\n", elm_holder->numElements()
    );
    elm_holder->foreach([col_msg, msg, handler](
      IndexT const& idx, Indexable<IndexT>* base
    ) {
      vtAssert(base != nullptr, "Must be valid pointer");

      // be very careful here, do not touch `base' after running the active
      // message because it might have migrated out and be invalid
      auto const from = col_msg->getFromNode();
      trace::TraceEventIDType trace_event = trace::no_trace_event;
      #if vt_check_enabled(trace_enabled)
        trace_event = col_msg->getFromTraceEvent();
      #endif
      collectionAutoMsgDeliver<ColT,IndexT,MsgT,typename MsgT::UserMsgType>(
        msg, base, handler, from, trace_event, false
      );
    });
  }
  /*
   *  Termination: consume for default epoch for correct termination: on the
   *  other end the sender produces p units for each broadcast to the default
   *  group
   */
  vt_debug_print(
    verbose, vrt_coll,
    "collectionBcastHandler: (consume) bcast_proxy={:x}, han={}, "
    "epoch={:x}, msg epoch={:x}, group={}, default group={}\n",
    bcast_proxy, col_msg->getVrtHandler(),
    cur_epoch, msg_epoch, group, default_group
  );
  theMsg()->popEpoch(cur_epoch);
}

template <typename ColT, typename IndexT, typename MsgT>
/*static*/ void CollectionManager::collectionMsgTypedHandler(MsgT* msg) {
  auto const col_msg = static_cast<CollectionMessage<ColT>*>(msg);
  auto const entity_proxy = col_msg->getProxy();
  auto const cur_epoch = theMsg()->getEpochContextMsg(msg);
  auto const& col = entity_proxy.getCollectionProxy();
  auto const& elm = entity_proxy.getElementProxy();
  auto const& idx = elm.getIndex();
  auto elm_holder = theCollection()->findElmHolder<IndexT>(col);

  bool const exists = elm_holder->exists(idx);

  vt_debug_print(
    terse, vrt_coll,
    "collectionMsgTypedHandler: exists={}, idx={}, cur_epoch={:x}\n",
    exists, idx, cur_epoch
  );

  vtAssertInfo(exists, "Proxy must exist", cur_epoch, idx);

  auto& inner_holder = elm_holder->lookup(idx);

  auto const sub_handler = col_msg->getVrtHandler();
  auto const col_ptr = inner_holder.getRawPtr();

  vt_debug_print(
    verbose, vrt_coll,
    "collectionMsgTypedHandler: sub_handler={}\n", sub_handler
  );

  vtAssertInfo(
    col_ptr != nullptr, "Must be valid pointer",
    sub_handler, HandlerManager::isHandlerMember(sub_handler), cur_epoch, idx, exists
  );

  // Dispatch the handler after pushing the contextual epoch
  theMsg()->pushEpoch(cur_epoch);
  auto const from = col_msg->getFromNode();

  trace::TraceEventIDType trace_event = trace::no_trace_event;
  #if vt_check_enabled(trace_enabled)
    trace_event = col_msg->getFromTraceEvent();
  #endif
  collectionAutoMsgDeliver<ColT,IndexT,MsgT,typename MsgT::UserMsgType>(
    msg, col_ptr, sub_handler, from, trace_event, false
  );
  theMsg()->popEpoch(cur_epoch);
}

template <typename ColT, typename MsgT>
/*static*/ void CollectionManager::recordStats(ColT* col_ptr, MsgT* msg) {
  auto const pfrom = msg->getSenderElm();

  if (pfrom.id == elm::no_element_id) {
    return;
  }

  auto const pto = col_ptr->getElmID();
  auto& stats = col_ptr->getStats();
  auto const msg_size = serialization::MsgSizer<MsgT>::get(msg);
  auto const cat = msg->getCat();
  vt_debug_print(
    normal, vrt_coll,
    "recordStats: receive msg: elm(to={}, from={}),"
    " no={}, size={}, category={}\n",
    pto, pfrom, elm::no_element_id, msg_size,
    static_cast<typename std::underlying_type<elm::CommCategory>::type>(cat)
  );
  if (
    cat == elm::CommCategory::SendRecv or
    cat == elm::CommCategory::Broadcast
  ) {
    bool bcast = cat == elm::CommCategory::SendRecv ? false : true;
    stats.recvObjData(pto, pfrom, msg_size, bcast);
  } else if (
    cat == elm::CommCategory::NodeToCollection or
    cat == elm::CommCategory::NodeToCollectionBcast
  ) {
    bool bcast = cat == elm::CommCategory::NodeToCollection ? false : true;
    auto nfrom = msg->getFromNode();
    stats.recvFromNode(pto, nfrom, msg_size, bcast);
  }
}

template <typename ColT, typename IndexT>
ColT* CollectionManager::getCollectionPtrForInvoke(
  VirtualElmProxyType<ColT> const& proxy
) {
  auto idx = proxy.getElementProxy().getIndex();
  auto elm_holder =
    theCollection()->findElmHolder<IndexT>(proxy.getCollectionProxy());

  vtAssert(elm_holder != nullptr, "Must have elm holder");
  vtAssert(
    elm_holder->exists(idx),
    fmt::format(
      "Element with idx:{} doesn't exist on node:{}\n", idx,
      theContext()->getNode()));

  auto& inner_holder = elm_holder->lookup(idx);

  return static_cast<ColT*>(inner_holder.getRawPtr());
}

template <typename ColT, typename Type, Type f, typename... Args>
util::Copyable<Type> CollectionManager::invoke(
  VirtualElmProxyType<ColT> const& proxy, Args... args
) {
  auto ptr = getCollectionPtrForInvoke(proxy);

  auto const this_node = theContext()->getNode();
  util::Copyable<Type> result;

  runnable::makeRunnableVoid(false, uninitialized_handler, this_node)
    .withCollection(ptr)
    .withLBStatsVoidMsg(ptr)
    .withExplicitTask([&]{
      result = runnable::invoke<Type, f>(ptr, std::forward<Args>(args)...);
    })
    .runOrEnqueue(true);

  return result;
}

template <typename ColT, typename Type, Type f, typename... Args>
util::NotCopyable<Type> CollectionManager::invoke(
  VirtualElmProxyType<ColT> const& proxy, Args... args
) {
  auto ptr = getCollectionPtrForInvoke(proxy);

  auto const this_node = theContext()->getNode();
  util::NotCopyable<Type> result;

  runnable::makeRunnableVoid(false, uninitialized_handler, this_node)
    .withCollection(ptr)
    .withLBStatsVoidMsg(ptr)
    .withExplicitTask([&]{
      auto&& ret = runnable::invoke<Type, f>(ptr, std::forward<Args>(args)...);
      result = std::move(ret);
    })
    .runOrEnqueue(true);

  return std::move(result);
}

template <typename ColT, typename Type, Type f, typename... Args>
util::IsVoidReturn<Type> CollectionManager::invoke(
  VirtualElmProxyType<ColT> const& proxy, Args... args
) {
  auto ptr = getCollectionPtrForInvoke(proxy);

  auto const this_node = theContext()->getNode();

  runnable::makeRunnableVoid(false, uninitialized_handler, this_node)
    .withCollection(ptr)
    .withLBStatsVoidMsg(ptr)
    .withExplicitTask([&]{
      runnable::invoke<Type, f>(ptr, std::forward<Args>(args)...);
    })
    .runOrEnqueue(true);
}

template <
  typename MsgT, ActiveColTypedFnType<MsgT, typename MsgT::CollectionType>* f
>
void CollectionManager::invokeMsg(
  VirtualElmProxyType<typename MsgT::CollectionType> const& proxy,
  messaging::MsgPtrThief<MsgT> msg, bool instrument
)
{
  using ColT = typename MsgT::CollectionType;

  auto& msgPtr = msg.msg_;
  msgPtr->setVrtHandler(
    auto_registry::makeAutoHandlerCollection<ColT, MsgT, f>()
  );

  invokeMsgImpl<ColT, MsgT>(proxy, msg, instrument);
}

template <
  typename MsgT,
  ActiveColMemberTypedFnType<MsgT, typename MsgT::CollectionType> f
>
void CollectionManager::invokeMsg(
  VirtualElmProxyType<typename MsgT::CollectionType> const& proxy,
  messaging::MsgPtrThief<MsgT> msg, bool instrument
)
{
  using ColT = typename MsgT::CollectionType;

  auto& msgPtr = msg.msg_;
  msgPtr->setVrtHandler(
    auto_registry::makeAutoHandlerCollectionMem<ColT, MsgT, f>()
  );

  invokeMsgImpl<ColT, MsgT>(proxy, msg, instrument);
}

template <typename ColT, typename MsgT>
void CollectionManager::invokeMsgImpl(
  VirtualElmProxyType<ColT> const& proxy, messaging::MsgPtrThief<MsgT> msg,
  bool instrument
)
{
  using IndexT = typename ColT::IndexType;
  auto& msgPtr = msg.msg_;

  auto idx = proxy.getElementProxy().getIndex();
  auto elm_holder =
    theCollection()->findElmHolder<IndexT>(proxy.getCollectionProxy()
  );

  vtAssert(elm_holder != nullptr, "Must have elm holder");
  vtAssert(
    elm_holder->exists(idx),
    fmt::format(
      "Element with idx:{} doesn't exist on node:{}\n", idx,
      theContext()->getNode()
    )
  );

#if vt_check_enabled(lblite)
  auto const elm_id = getCurrentContext();

  vt_debug_print(
    verbose, vrt_coll,
    "invokeMsg: LB current elm context elm={}\n",
    elm_id
  );

  if (elm_id.id != elm::no_element_id) {
    msgPtr->setSenderElm(elm_id);
    msgPtr->setCat(elm::CommCategory::LocalInvoke);
  }
#endif

  auto const cur_epoch = theMsg()->setupEpochMsg(msgPtr);
  auto& inner_holder = elm_holder->lookup(idx);
  auto const col_ptr = inner_holder.getRawPtr();
  auto const from = theContext()->getNode();

  msgPtr->setFromNode(from);
  msgPtr->setProxy(proxy);

  theMsg()->pushEpoch(cur_epoch);

  auto const han = msgPtr->getVrtHandler();

  trace::TraceEventIDType trace_event = trace::no_trace_event;
#if vt_check_enabled(trace_enabled)

  auto const msg_size = vt::serialization::MsgSizer<MsgT>::get(msgPtr.get());
  const bool is_bcast = false;

  trace_event = theMsg()->makeTraceCreationSend(han, msg_size, is_bcast);
#endif

  collectionAutoMsgDeliver<ColT, IndexT, MsgT, typename MsgT::UserMsgType>(
    msgPtr.get(), col_ptr, han, from, trace_event, true
  );

  theMsg()->popEpoch(cur_epoch);
}

template <typename ColT, typename IndexT>
/*static*/ void CollectionManager::collectionMsgHandler(BaseMessage* msg) {
  return collectionMsgTypedHandler<ColT,IndexT,CollectionMessage<ColT>>(
    static_cast<CollectionMessage<ColT>*>(msg)
  );
}

template <typename ColT, typename IndexT, typename MsgT>
/*static*/ void CollectionManager::broadcastRootHandler(MsgT* msg) {
  envelopeUnlockForForwarding(msg->env);
  theCollection()->broadcastFromRoot<ColT,IndexT,MsgT>(msg);
}

template <typename ColT, typename IndexT, typename MsgT>
messaging::PendingSend CollectionManager::broadcastFromRoot(MsgT* raw_msg) {
  auto msg = promoteMsg(raw_msg);

  // broadcast to all nodes
  auto const& this_node = theContext()->getNode();
  auto const& proxy = msg->getBcastProxy();
  auto elm_holder = theCollection()->findElmHolder<IndexT>(proxy);
  auto const bcast_node = VirtualProxyBuilder::getVirtualNode(proxy);

  vtAssert(elm_holder != nullptr, "Must have elm holder");
  vtAssert(this_node == bcast_node, "Must be the bcast node");

  auto const cur_epoch = theMsg()->getEpochContextMsg(msg);
  theMsg()->pushEpoch(cur_epoch);

  vt_debug_print(
    normal, vrt_coll,
    "broadcastFromRoot: proxy={:x}, han={}\n",
    proxy, msg->getVrtHandler()
  );

  auto const& group_ready = elm_holder->groupReady();
  auto const& use_group = elm_holder->useGroup();
  bool const send_group = group_ready && use_group;

  vt_debug_print(
    verbose, vrt_coll,
    "broadcastFromRoot: proxy={:x}, han={}, group_ready={}, "
    "group_active={}, use_group={}, send_group={}, group={:x}, cur_epoch={:x}\n",
    proxy, msg->getVrtHandler(),
    group_ready, send_group, use_group, send_group,
    use_group ? elm_holder->group() : default_group, cur_epoch
  );

  if (send_group) {
    auto const& group = elm_holder->group();
    envelopeSetGroup(msg->env, group);
  }

  theMsg()->markAsCollectionMessage(msg);
  auto msg_hold = promoteMsg(msg.get()); // keep after bcast
  auto ret = theMsg()->broadcastMsg<MsgT,collectionBcastHandler<ColT,IndexT>>(
    msg
  );

  theMsg()->popEpoch(cur_epoch);

  return ret;
}

template <
  typename MsgT, ActiveColTypedFnType<MsgT, typename MsgT::CollectionType>* f
>
messaging::PendingSend CollectionManager::broadcastCollectiveMsg(
  CollectionProxyWrapType<typename MsgT::CollectionType> const& proxy,
  messaging::MsgPtrThief<MsgT> msg, bool instrument
) {
  using ColT = typename MsgT::CollectionType;

  auto& msgPtr = msg.msg_;
  msgPtr->setVrtHandler(auto_registry::makeAutoHandlerCollection<ColT, MsgT, f>());

  return broadcastCollectiveMsgImpl<MsgT, ColT>(proxy, msgPtr, instrument);
}

template <
  typename MsgT,
  ActiveColMemberTypedFnType<MsgT, typename MsgT::CollectionType> f
>
messaging::PendingSend CollectionManager::broadcastCollectiveMsg(
  CollectionProxyWrapType<typename MsgT::CollectionType> const& proxy,
  messaging::MsgPtrThief<MsgT> msg, bool instrument
) {
  using ColT = typename MsgT::CollectionType;

  auto& msgPtr = msg.msg_;
  msgPtr->setVrtHandler(
    auto_registry::makeAutoHandlerCollectionMem<ColT, MsgT, f>()
  );

  return broadcastCollectiveMsgImpl<MsgT, ColT>(proxy, msgPtr, instrument);
}

template <typename MsgT, typename ColT>
messaging::PendingSend CollectionManager::broadcastCollectiveMsgImpl(
  CollectionProxyWrapType<ColT> const& proxy, MsgPtr<MsgT>& msg, bool instrument
) {
  using IndexT = typename ColT::IndexType;

  msg->setFromNode(theContext()->getNode());
  msg->setBcastProxy(proxy.getProxy());

#if vt_check_enabled(trace_enabled)
  // Create the trace creation event for the broadcast here to connect it a
  // higher semantic level
  auto const han = msg->getVrtHandler();
  auto const msg_size = vt::serialization::MsgSizer<MsgT>::get(msg.get());
  const bool is_bcast = true;

  auto event = theMsg()->makeTraceCreationSend(han, msg_size, is_bcast);
  msg->setFromTraceEvent(event);
#endif

#if vt_check_enabled(lblite)
  msg->setLBLiteInstrument(instrument);
  msg->setCat(elm::CommCategory::CollectiveToCollectionBcast);
#endif

  theMsg()->markAsCollectionMessage(msg);
  collectionBcastHandler<ColT, IndexT, MsgT>(msg.get());

  return messaging::PendingSend{nullptr};
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

template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT, ColT>* f>
CollectionManager::IsNotColMsgType<MsgT> CollectionManager::broadcastMsg(
  CollectionProxyWrapType<ColT> const& proxy, MsgT* msg, bool instrument
) {
  auto const& h = auto_registry::makeAutoHandlerCollection<ColT, MsgT, f>();
  return broadcastNormalMsg<MsgT, ColT>(proxy, msg, h, instrument);
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
  typename MsgT, typename ColT, ActiveColMemberTypedFnType<MsgT, ColT> f
>
CollectionManager::IsNotColMsgType<MsgT> CollectionManager::broadcastMsg(
  CollectionProxyWrapType<ColT> const& proxy, MsgT* msg, bool instrument
) {
  auto const& h = auto_registry::makeAutoHandlerCollectionMem<ColT, MsgT, f>();
  return broadcastNormalMsg<MsgT, ColT>(proxy, msg, h, instrument);
}

template <
  typename MsgT, typename ColT, ActiveColMemberTypedFnType<MsgT, ColT> f
>
messaging::PendingSend CollectionManager::broadcastMsgImpl(
  CollectionProxyWrapType<ColT> const& proxy, MsgT* const msg, bool inst
) {
  // register the user's handler
  auto const& h = auto_registry::makeAutoHandlerCollectionMem<ColT, MsgT, f>();
  return broadcastMsgUntypedHandler<MsgT>(proxy, msg, h, inst);
}

template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT, ColT>* f>
messaging::PendingSend CollectionManager::broadcastMsgImpl(
  CollectionProxyWrapType<ColT> const& proxy, MsgT* const msg, bool inst
) {
  // register the user's handler
  auto const& h = auto_registry::makeAutoHandlerCollection<ColT, MsgT, f>();
  return broadcastMsgUntypedHandler<MsgT>(proxy, msg, h, inst);
}

template <typename MsgT, typename ColT>
CollectionManager::IsColMsgType<MsgT> CollectionManager::broadcastMsgWithHan(
  CollectionProxyWrapType<ColT> const& proxy, MsgT* msg, HandlerType const h,
  bool inst
) {
  using IdxT = typename ColT::IndexType;
  return broadcastMsgUntypedHandler<MsgT, ColT, IdxT>(proxy, msg, h, inst);
}

template <typename MsgT, typename ColT>
CollectionManager::IsNotColMsgType<MsgT> CollectionManager::broadcastMsgWithHan(
  CollectionProxyWrapType<ColT> const& proxy, MsgT* msg, HandlerType const h,
  bool inst
) {
  return broadcastNormalMsg<MsgT, ColT>(proxy, msg, h, inst);
}

template <typename MsgT, typename ColT>
messaging::PendingSend CollectionManager::broadcastNormalMsg(
  CollectionProxyWrapType<ColT> const& proxy, MsgT* msg,
  HandlerType const handler, bool instrument
) {
  auto wrap_msg = makeMessage<ColMsgWrap<ColT, MsgT>>(*msg);
  return broadcastMsgUntypedHandler<ColMsgWrap<ColT, MsgT>, ColT>(
    proxy, wrap_msg.get(), handler, instrument
  );
}

template <typename MsgT, typename ColT, typename IdxT>
messaging::PendingSend CollectionManager::broadcastMsgUntypedHandler(
  CollectionProxyWrapType<ColT, IdxT> const& toProxy, MsgT* raw_msg,
  HandlerType const handler, bool instrument
) {
  auto const idx = makeVrtDispatch<MsgT,ColT>();
  auto const col_proxy = toProxy.getProxy();
  auto msg = promoteMsg(raw_msg);

  vt_debug_print(
    normal, vrt_coll,
    "broadcastMsgUntypedHandler: msg={}, idx={}\n",
    print_ptr(msg.get()), idx
  );

  // save the user's handler in the message
  msg->setFromNode(theContext()->getNode());
  msg->setVrtHandler(handler);
  msg->setBcastProxy(col_proxy);

# if vt_check_enabled(trace_enabled)
  // Create the trace creation event for the broadcast here to connect it a
  // higher semantic level
  auto const msg_size = vt::serialization::MsgSizer<MsgT>::get(msg.get());
  const bool is_bcast = true;
  auto event = theMsg()->makeTraceCreationSend(handler, msg_size, is_bcast);
  msg->setFromTraceEvent(event);
# endif

# if vt_check_enabled(lblite)
  msg->setLBLiteInstrument(instrument);
  auto const elm_id = getCurrentContext();

  vt_debug_print(
    verbose, vrt_coll,
    "broadcasting msg: LB current elm context elm={}\n",
    elm_id
  );

  if (elm_id.id != elm::no_element_id) {
    msg->setSenderElm(elm_id);
    msg->setCat(elm::CommCategory::Broadcast);
  }
# endif

  auto const cur_epoch = theMsg()->setupEpochMsg(msg);
  auto const this_node = theContext()->getNode();
  auto const bnode = VirtualProxyBuilder::getVirtualNode(col_proxy);

  if (this_node != bnode) {
    theMsg()->pushEpoch(cur_epoch);
    vt_debug_print(
      verbose, vrt_coll,
      "broadcastMsgUntypedHandler: col_proxy={:x}, sending to root node={}, "
      "handler={}, cur_epoch={:x}\n",
      col_proxy, bnode, handler, cur_epoch
    );
    theMsg()->markAsCollectionMessage(msg);

    auto pmsg = std::move(msg); // sendMsg needs non-const variable
    auto ps = theMsg()->sendMsg<MsgT,broadcastRootHandler<ColT,IdxT,MsgT>>(
      bnode, pmsg
    );
    theMsg()->popEpoch(cur_epoch);
    return ps;
  } else {
    theMsg()->pushEpoch(cur_epoch);
    vt_debug_print(
      verbose, vrt_coll,
      "broadcasting msg to collection: msg={}, handler={}\n",
      print_ptr(msg.get()), handler
    );
    auto ps = broadcastFromRoot<ColT,IdxT,MsgT>(msg.get());
    theMsg()->popEpoch(cur_epoch);
    return ps;
  }
}

template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
messaging::PendingSend CollectionManager::reduceMsgExpr(
  CollectionProxyWrapType<ColT> const& proxy,
  MsgT *const raw_msg, ReduceIdxFuncType<typename ColT::IndexType> expr_fn,
  ReduceStamp stamp, NodeType root
) {
  using IndexT = typename ColT::IndexType;

  vtAssert(
    hasContext<IndexT>(), "Must have collection element context"
  );
  vtAssert(
    queryProxyContext<IndexT>() == proxy.getProxy(),
    "Must have matching proxy context"
  );

  // Get the current running index context
  IndexT idx = *queryIndexContext<IndexT>();

  auto msg = promoteMsg(raw_msg);

  vt_debug_print(
    terse, vrt_coll,
    "reduceMsg: msg={}\n", print_ptr(raw_msg)
  );

  auto const col_proxy = proxy.getProxy();
  auto const cur_epoch = theMsg()->getEpochContextMsg(msg);

  theMsg()->pushEpoch(cur_epoch);
  auto elm_holder = findElmHolder<IndexT>(col_proxy);

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

  auto cur_stamp = stamp;
  if (cur_stamp == ReduceStamp{}) {
    cur_stamp = proxy(idx).tryGetLocalPtr()->getNextStamp();
  }

  collective::reduce::Reduce* r = nullptr;
  if (use_group) {
    r = theGroup()->groupReducer(group);
  } else {
    r = theCollective()->getReducerVrtProxy(col_proxy);
  }

  r->reduceImmediate<MsgT,f>(root_node, msg.get(), cur_stamp, num_elms);

  vt_debug_print(
    normal, vrt_coll,
    "reduceMsg: col_proxy={:x}, num_elms={}\n",
    col_proxy, num_elms
  );

  theMsg()->popEpoch(cur_epoch);
  return messaging::PendingSend{nullptr};
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
  auto const mapped_node = getMappedNode<ColT>(proxy, idx);
  return reduceMsgExpr<ColT,MsgT,f>(proxy,msg,nullptr,stamp,mapped_node);
}

template <typename MsgT, typename ColT>
CollectionManager::IsNotColMsgType<MsgT> CollectionManager::sendMsgWithHan(
  VirtualElmProxyType<ColT> const& proxy, MsgT* msg,
  HandlerType const handler
) {
  return sendNormalMsg<MsgT, ColT>(proxy, msg, handler);
}

template <typename MsgT, typename ColT>
CollectionManager::IsColMsgType<MsgT> CollectionManager::sendMsgWithHan(
  VirtualElmProxyType<ColT> const& proxy, MsgT* msg,
  HandlerType const handler
) {
  using IdxT = typename ColT::IndexType;
  return sendMsgUntypedHandler<MsgT, ColT, IdxT>(proxy, msg, handler);
}

template <typename MsgT, typename ColT>
messaging::PendingSend CollectionManager::sendNormalMsg(
  VirtualElmProxyType<ColT> const& proxy, MsgT* msg,
  HandlerType const handler
) {
  auto wrap_msg = makeMessage<ColMsgWrap<ColT, MsgT>>(*msg);
  return sendMsgUntypedHandler<ColMsgWrap<ColT, MsgT>, ColT>(
    proxy, wrap_msg.get(), handler
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

template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT, ColT>* f>
CollectionManager::IsNotColMsgType<MsgT>
CollectionManager::sendMsg(VirtualElmProxyType<ColT> const& proxy, MsgT* msg) {
  auto const& h = auto_registry::makeAutoHandlerCollection<ColT, MsgT, f>();
  return sendNormalMsg<MsgT, ColT>(proxy, msg, h);
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
  typename MsgT, typename ColT, ActiveColMemberTypedFnType<MsgT, ColT> f
>
CollectionManager::IsNotColMsgType<MsgT>
CollectionManager::sendMsg(VirtualElmProxyType<ColT> const& proxy, MsgT* msg) {
  auto const& h = auto_registry::makeAutoHandlerCollectionMem<ColT, MsgT, f>();
  return sendNormalMsg<MsgT, ColT>(proxy, msg, h);
}

template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT, ColT>* f>
messaging::PendingSend CollectionManager::sendMsgImpl(
  VirtualElmProxyType<ColT> const& proxy, MsgT* msg
) {
  auto const& h = auto_registry::makeAutoHandlerCollection<ColT, MsgT, f>();
  return sendMsgUntypedHandler<MsgT>(proxy, msg, h);
}

template <
  typename MsgT, typename ColT,
  ActiveColMemberTypedFnType<MsgT, typename MsgT::CollectionType> f
>
messaging::PendingSend CollectionManager::sendMsgImpl(
  VirtualElmProxyType<ColT> const& proxy, MsgT* msg
) {
  auto const& h = auto_registry::makeAutoHandlerCollectionMem<ColT, MsgT, f>();
  return sendMsgUntypedHandler<MsgT>(proxy, msg, h);
}

template <typename MsgT, typename ColT, typename IdxT>
messaging::PendingSend CollectionManager::sendMsgUntypedHandler(
  VirtualElmProxyType<ColT> const& toProxy, MsgT* raw_msg,
  HandlerType const handler, bool imm_context
) {
  auto const& col_proxy = toProxy.getCollectionProxy();
  auto const& elm_proxy = toProxy.getElementProxy();

  auto msg = promoteMsg(raw_msg);

# if vt_check_enabled(lblite)
  msg->setLBLiteInstrument(true);

  auto const elm_id = getCurrentContext();

  vt_debug_print(
    normal, vrt_coll,
    "sending msg: LB current elm context elm={}\n",
    elm_id
  );

  if (elm_id.id != elm::no_element_id) {
    msg->setSenderElm(elm_id);
    msg->setCat(elm::CommCategory::SendRecv);
  }
# endif

  // set bit so it isn't recorded as it routes through bare handlers
  envelopeSetCommStatsRecordedAboveBareHandler(msg->env, true);

# if vt_check_enabled(trace_enabled)
  // Create the trace creation event here to connect it a higher semantic
  // level. Do it in the imm_context so we see the send event when the user
  // actually invokes send on the proxy (not outside the event that actually
  // sent it)
  auto const msg_size = vt::serialization::MsgSizer<MsgT>::get(msg.get());
  bool const is_bcast = false;
  auto const event = theMsg()->makeTraceCreationSend(handler,  msg_size, is_bcast);
  msg->setFromTraceEvent(event);
#endif

  auto const cur_epoch = theMsg()->setupEpochMsg(msg);
  msg->setFromNode(theContext()->getNode());
  msg->setVrtHandler(handler);
  msg->setProxy(toProxy);

  auto idx = elm_proxy.getIndex();
  vt_debug_print(
    terse, vrt_coll,
    "sendMsgUntypedHandler: col_proxy={:x}, cur_epoch={:x}, idx={}, "
    "handler={}, imm_context={}\n",
    col_proxy, cur_epoch, idx, handler, imm_context
  );

  return messaging::PendingSend{
    msg, [=](MsgSharedPtr<BaseMsgType>& inner_msg){
      theMsg()->pushEpoch(cur_epoch);
      auto home_node = theCollection()->getMappedNode<ColT>(col_proxy, idx);
      // route the message to the destination using the location manager
      auto lm = theLocMan()->getCollectionLM<IdxT>(col_proxy);
      vtAssert(lm != nullptr, "LM must exist");
      theMsg()->markAsCollectionMessage(msg);
      lm->template routeMsgSerializeHandler<
        MsgT, collectionMsgTypedHandler<ColT,IdxT,MsgT>
      >(idx, home_node, msg);
      theMsg()->popEpoch(cur_epoch);
    }
  };
}

template <typename ColT, typename IndexT>
bool CollectionManager::insertCollectionElement(
  VirtualPtrType<IndexT> vc, VirtualProxyType const proxy,
  IndexT const& idx, NodeType const home_node, bool const is_migrated_in,
  NodeType const migrated_from
) {
  auto holder = findColHolder<IndexT>(proxy);

  vt_debug_print(
    terse, vrt_coll,
    "insertCollectionElement: proxy={:x}, idx={}\n",
    proxy, print_index(idx)
  );

  vtAssert(holder != nullptr, "Must have meta-data before inserting");

  auto elm_holder = findElmHolder<IndexT>(proxy);
  auto const elm_exists = elm_holder->exists(idx);

  vt_debug_print(
    normal, vrt_coll,
    "insertCollectionElement: elm_exists={}, proxy={:x}, idx={}\n",
    elm_exists, proxy, idx
  );

  if (elm_exists) {
    return false;
  }
  vtAssert(!elm_exists, "Must not exist at this point");

  auto const destroyed = elm_holder->isDestroyed();

  vt_debug_print(
    normal, vrt_coll,
    "insertCollectionElement: destroyed={}, proxy={:x}, idx={}\n",
    destroyed, proxy, idx
  );

  if (!destroyed) {
    elm_holder->insert(idx, typename Holder<IndexT>::InnerHolder{
      std::move(vc)
    });

    if (is_migrated_in) {
      theLocMan()->getCollectionLM<IndexT>(proxy)->entityImmigrated(
        idx, home_node, migrated_from,
        CollectionManager::collectionMsgHandler<ColT, IndexT>
      );
      elm_holder->applyListeners(
        listener::ElementEventEnum::ElementMigratedIn, idx, home_node
      );
    } else {
      theLocMan()->getCollectionLM<IndexT>(proxy)->registerEntity(
        idx, home_node,
        CollectionManager::collectionMsgHandler<ColT, IndexT>
      );
      elm_holder->applyListeners(
        listener::ElementEventEnum::ElementCreated, idx, home_node
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
CollectionManager::IsDefaultConstructableType<ColT>
CollectionManager::constructCollective(typename ColT::IndexType range) {
  auto const map_han = getDefaultMap<ColT>();
  auto cons_fn = [](typename ColT::IndexType){return std::make_unique<ColT>();};
  return constructCollectiveMap<ColT>(range,cons_fn,map_han);
}

template <typename ColT>
CollectionManager::CollectionProxyWrapType<ColT>
CollectionManager::constructCollective(
  typename ColT::IndexType range, DistribConstructFn<ColT> cons_fn
) {
  auto const map_han = getDefaultMap<ColT>();
  return constructCollectiveMap<ColT>(range,cons_fn,map_han);
}

template <
  typename ColT,  mapping::ActiveMapTypedFnType<typename ColT::IndexType> fn
>
CollectionManager::IsDefaultConstructableType<ColT, typename ColT::IndexType>
CollectionManager::constructCollective(typename ColT::IndexType range) {
  using IndexT = typename ColT::IndexType;
  auto cons_fn = [](typename ColT::IndexType){return std::make_unique<ColT>();};
  auto const& map_han = auto_registry::makeAutoHandlerMap<IndexT, fn>();
  return constructCollectiveMap<ColT>(range,cons_fn,map_han);
}

template <
  typename ColT,  mapping::ActiveMapTypedFnType<typename ColT::IndexType> fn
>
CollectionManager::CollectionProxyWrapType<ColT, typename ColT::IndexType>
CollectionManager::constructCollective(
  typename ColT::IndexType range, DistribConstructFn<ColT> cons_fn
) {
  using IndexT = typename ColT::IndexType;
  auto const& map_han = auto_registry::makeAutoHandlerMap<IndexT, fn>();
  return constructCollectiveMap<ColT>(range,cons_fn,map_han);
}

template <typename ColT>
CollectionManager::CollectionProxyWrapType<ColT>
CollectionManager::constructCollectiveMap(
  typename ColT::IndexType range, DistribConstructFn<ColT> user_construct_fn,
  HandlerType const map_han
) {
  return vt::makeCollection<ColT>()
    .bounds(range)
    .bulkInsert()
    .mapperHandler(map_han)
    .elementConstructor(user_construct_fn)
    .wait();
}

/* end SPMD distributed collection support */

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
  using IdxContextHolder = CollectionContextHolder<IndexT>;
  return IdxContextHolder::index();
}

template <typename IndexT>
/*static*/ VirtualProxyType CollectionManager::queryProxyContext() {
  using IdxContextHolder = CollectionContextHolder<IndexT>;
  return IdxContextHolder::proxy();
}

template <typename IndexT>
/*static*/ bool CollectionManager::hasContext() {
  using IdxContextHolder = CollectionContextHolder<IndexT>;
  return IdxContextHolder::hasContext();
}

template <typename ColT, typename... Args>
void CollectionManager::insertMetaCollection(
  VirtualProxyType const& proxy, Args&&... args
) {
  using IndexType      = typename ColT::IndexType;
  using MetaHolderType = EntireHolder<IndexType>;
  using HolderType     = typename MetaHolderType::InnerHolder;

  // Create and insert the meta-data into the meta-collection holder
  auto holder = std::make_shared<HolderType>(std::forward<Args>(args)...);
  MetaHolderType::insert(proxy,holder);
  typeless_holder_.insertCollectionInfo(proxy, holder, [proxy]{
    theCollection()->constructGroup<ColT>(proxy);
  });
  /*
   *  This is to ensure that the collection LM instance gets created so that
   *  messages can be forwarded properly
   */
  theLocMan()->getCollectionLM<IndexType>(proxy);

  /**
   * Type-erase some lambdas for doing the collective broadcast that collects up
   * the statistics on each node for each collection element
   */
  collect_stats_for_lb_[proxy] = [bits=proxy]{
    using namespace balance;
    using MsgType = CollectStatsMsg<ColT>;
    auto const phase = thePhase()->getCurrentPhase();
    CollectionProxyWrapType<ColT> p{bits};
    p.template broadcastCollective<MsgType,CollectionStats::syncNextPhase<ColT>>(
      phase
    );
  };
}

template <typename ColT>
void CollectionManager::constructGroup(VirtualProxyType const& proxy) {
  /*
   *  Create a new group for the collection that only contains the nodes for
   *  which elements exist. If the collection is static, this group will never
   *  change
   */

  auto const elms = groupElementCount<ColT>(proxy);
  bool const in_group = elms > 0;

  vt_debug_print(
    normal, vrt_coll,
    "constructGroup: creating new group: proxy={:x}, elms={}, in_group={}\n",
    proxy, elms, in_group
  );

  createGroupCollection<ColT>(proxy, in_group);
}


template <typename ColT>
CollectionManager::CollectionProxyWrapType<ColT, typename ColT::IndexType>
CollectionManager::construct(typename ColT::IndexType range) {
  auto const map_han = getDefaultMap<ColT>();
  return constructMap<ColT>(range,map_han);
}

template <
  typename ColT, mapping::ActiveMapTypedFnType<typename ColT::IndexType> fn
>
CollectionManager::CollectionProxyWrapType<ColT, typename ColT::IndexType>
CollectionManager::construct(typename ColT::IndexType range) {
  using IndexT = typename ColT::IndexType;
  auto const& map_han = auto_registry::makeAutoHandlerMap<IndexT, fn>();
  return constructMap<ColT>(range, map_han);
}

template <typename ColT>
CollectionManager::CollectionProxyWrapType<ColT, typename ColT::IndexType>
CollectionManager::constructMap(
  typename ColT::IndexType range, HandlerType const map_handler
) {
  return vt::makeCollection<ColT>()
    .bounds(range)
    .bulkInsert()
    .collective(false)
    .mapperHandler(map_handler)
    .wait();
}

inline void CollectionManager::insertCollectionInfo(
  VirtualProxyType const& proxy, HandlerType const map_han
) {
  typeless_holder_.insertMap(proxy,map_han);
}

/*
 * Support of virtual context collection element dynamic insertion
 */

template <typename ColT, typename MsgT>
/*static*/ void CollectionManager::insertHandler(InsertMsg<ColT, MsgT>* msg) {
  auto const insert_epoch = msg->insert_epoch_;
  ModifierToken token{insert_epoch};
  theCollection()->insert<ColT, MsgT>(
    msg->proxy_, msg->idx_, msg->construct_node_, token, msg->insert_msg_,
    msg->pinged_
  );
}

template <typename ColT, typename MsgT>
/*static*/ void CollectionManager::pingHomeHandler(InsertMsg<ColT, MsgT>* msg) {
  using IndexType = typename ColT::IndexType;
  auto proxy = msg->proxy_;
  auto idx = msg->idx_;
  auto lm = theLocMan()->getCollectionLM<IndexType>(proxy.getProxy());
  auto elm_lives_somewhere = lm->isCached(idx);

  vt_debug_print(
    verbose, vrt_coll,
    "pingHomeHandler: idx={}, elm_lives_somewhere={}\n",
    idx, elm_lives_somewhere
  );

  if (elm_lives_somewhere) {
    // send no message back---cancel the insertion
  } else {
    auto const insert_node = msg->construct_node_;
    // reserve the slot to stop any race with other insertions
    lm->registerEntityRemote(idx, msg->home_node_, insert_node);

    // send a message back that the insertion shall proceed
    auto send_msg = makeMessage<InsertMsg<ColT, MsgT>>(
      msg->proxy_, msg->idx_, msg->construct_node_, msg->home_node_,
      msg->insert_epoch_, msg->insert_msg_
    );
    send_msg->pinged_ = true;
    theMsg()->markAsCollectionMessage(send_msg);
    theMsg()->sendMsg<InsertMsg<ColT, MsgT>,insertHandler<ColT, MsgT>>(
      insert_node, send_msg
    );
  }
}

template <typename IdxT>
NodeType CollectionManager::getMappedNode(
  VirtualProxyType proxy, IdxT const& idx
) {
  auto col_holder = findColHolder<IdxT>(proxy);
  auto map_han = col_holder->map_fn;
  auto map_object = col_holder->map_object;
  auto bounds = col_holder->bounds;
  return getElementMapping(map_han, map_object, idx, bounds);
}

template <typename ColT>
NodeType CollectionManager::getMappedNode(
  CollectionProxyWrapType<ColT> const& proxy,
  typename ColT::IndexType const& idx
) {
  auto const untyped_proxy = proxy.getProxy();
  return getMappedNode<typename ColT::IndexType>(untyped_proxy, idx);
}

template <typename ColT, typename IndexT>
ColT* CollectionManager::tryGetLocalPtr(
  CollectionProxyWrapType<ColT,IndexT> const& proxy, IndexT idx
) {
  auto const untyped = proxy.getProxy();
  auto elm_holder = findElmHolder<IndexT>(untyped);

   vtAssert(
     elm_holder != nullptr, "Collection must be registered here"
   );

   auto const& elm_exists = elm_holder->exists(idx);

   if (elm_exists) {
     auto& elm_info = elm_holder->lookup(idx);
     auto elm_ptr = elm_info.getRawPtr();

     vtAssert(
       elm_ptr != nullptr, "Pointer to the element must not be nullptr"
     );

     return static_cast<ColT*>(elm_ptr);
   } else {
     return nullptr;
   }
}

template <typename ColT>
ModifierToken CollectionManager::beginModification(
  CollectionProxyWrapType<ColT> const& proxy, std::string const& label
) {
  auto epoch = theTerm()->makeEpochCollective(label);

  vt_debug_print(
    normal, vrt_coll,
    "beginModification: label={}, epoch={:x}\n", label, epoch
  );

  ModifierToken token{epoch};
  return token;
}

template <typename ColT>
void CollectionManager::finishModification(
  CollectionProxyWrapType<ColT> const& proxy, ModifierToken&& token
) {
  using IndexType = typename ColT::IndexType;

  auto untyped_proxy = proxy.getProxy();

  auto const epoch = token.modifyEpoch();

  vt_debug_print(
    normal, vrt_coll,
    "finishModification: epoch={:x}\n", epoch
  );

  theTerm()->finishedEpoch(epoch);
  runSchedulerThrough(epoch);

  // Compute the proper reduce stamp for the insertions that took place
  using StrongSeq = collective::reduce::detail::StrongSeq;
  using SeqType = typename StrongSeq::Type;

  auto elm_holder = findElmHolder<IndexType>(untyped_proxy);
  SeqType min_seq = std::numeric_limits<SeqType>::max();
  elm_holder->foreach([&](IndexType const&, Indexable<IndexType>* c) {
    // skip zeros since they are insertions that just happened
    if (*c->reduce_stamp_ != 0) {
      min_seq = std::min(*c->reduce_stamp_, min_seq);
    }
  });

  // Compute the global min stamp with a reduction
  auto r = theCollection()->reducer();

  using collective::reduce::makeStamp;
  using collective::reduce::StrongUserID;

  NodeType collective_root = 0;
  auto stamp = makeStamp<StrongUserID>(untyped_proxy);
  auto msg = makeMessage<CollectionStampMsg>(untyped_proxy, min_seq);
  auto cb = theCB()->makeBcast<
    CollectionStampMsg,&CollectionManager::computeReduceStamp
  >();
  r->reduce<collective::MinOp<SeqType>>(collective_root, msg.get(), cb, stamp);

  theSched()->runSchedulerWhile([untyped_proxy]{
    return theCollection()->reduce_stamp_.find(untyped_proxy) ==
           theCollection()->reduce_stamp_.end();
  });

  auto iter = reduce_stamp_.find(untyped_proxy);
  vtAssert(iter != reduce_stamp_.end(), "Must have value");

  SeqType const global_min_stamp = iter->second;
  reduce_stamp_.erase(iter);

  // set all new insertions to proper value
  elm_holder->foreach([&](IndexType const&, Indexable<IndexType>* c) {
    if (*c->reduce_stamp_ == 0) {
      *c->reduce_stamp_ = global_min_stamp;
    }
  });

  vt_debug_print(
    verbose, vrt_coll,
    "finishedInserting: creating new group\n"
  );

  runInEpochCollective([&]{ constructGroup<ColT>(untyped_proxy); });
}

namespace detail {

template <typename MsgT, typename ColT, typename Enable_ = void>
struct InsertMsgDispatcher;

template <typename MsgT, typename ColT>
struct InsertMsgDispatcher<
  MsgT, ColT,
  std::enable_if_t<std::is_same<MsgT, InsertNullMsg>::value>
> {
  static CollectionManager::DistribConstructFn<ColT> makeCons(MsgSharedPtr<MsgT>) {
    using IndexType = typename ColT::IndexType;
    return [](IndexType){ return std::make_unique<ColT>(); };
  }
};

template <typename MsgT, typename ColT>
struct InsertMsgDispatcher<
  MsgT, ColT,
  std::enable_if_t<not std::is_same<MsgT, InsertNullMsg>::value>
> {
  static CollectionManager::DistribConstructFn<ColT> makeCons(
    MsgSharedPtr<MsgT> msg
  ) {
    using IndexType = typename ColT::IndexType;
    return [=](IndexType){ return std::make_unique<ColT>(msg.get()); };
  }
};

} /* end namespace detail */

template <typename ColT, typename MsgT>
void CollectionManager::insert(
  CollectionProxyWrapType<ColT> const& proxy, typename ColT::IndexType idx,
  NodeType const node, ModifierToken& token, MsgSharedPtr<MsgT> insert_msg,
  bool pinged_home_already
) {
  using IndexType = typename ColT::IndexType;

  auto const modify_epoch = token.modifyEpoch();
  auto const untyped_proxy = proxy.getProxy();
  vtAssert(modify_epoch != no_epoch, "Modification epoch should be valid");

  vt_debug_print(
    normal, vrt_coll,
    "insert: proxy={:x}\n",
    untyped_proxy
  );

  theMsg()->pushEpoch(modify_epoch);

  auto const mapped_node = getMappedNode<ColT>(proxy, idx);
  auto const has_explicit_node = node != uninitialized_destination;
  auto const insert_node = has_explicit_node ? node : mapped_node;
  auto const this_node = theContext()->getNode();

  bool proceed_with_insertion = true;

  vt_debug_print(
    normal, vrt_coll,
    "insert: insert_node={}, mapped_node={}\n",
    insert_node, mapped_node
  );

  if (insert_node == this_node and not pinged_home_already) {
    // Case 0--insertion from home node onto home node (or message sent from
    // another node to insert on home node)
    if (mapped_node == this_node) {
      auto elm_holder = findElmHolder<IndexType>(untyped_proxy);
      auto const elm_exists = elm_holder->exists(idx);
      if (elm_exists) {
        // element exists here and is live--return
        proceed_with_insertion = false;
      } else {
        auto lm = theLocMan()->getCollectionLM<IndexType>(untyped_proxy);
        auto elm_lives_somewhere = lm->isCached(idx);
        if (elm_lives_somewhere) {
          // element exists somewhere in the system and since we are the home
          // we check the cache to determine if it has been inserted
          proceed_with_insertion = false;
        }
      }
    } else {
      // Case 1: insertion from the non-home node---we must check if the home
      // has a reserved entry from another insertion. If so, we cancel the
      // insertion---otherwise, we reserve for this insertion
      auto msg = makeMessage<InsertMsg<ColT, MsgT>>(
        proxy, idx, insert_node, mapped_node, modify_epoch, insert_msg
      );
      theMsg()->markAsCollectionMessage(msg);
      theMsg()->sendMsg<InsertMsg<ColT, MsgT>, pingHomeHandler<ColT, MsgT>>(
        mapped_node, msg
      );
      proceed_with_insertion = false;
    }
  }

  if (insert_node == this_node and proceed_with_insertion) {
    auto cons_fn = detail::InsertMsgDispatcher<MsgT, ColT>::makeCons(insert_msg);
    makeCollectionElement<ColT>(untyped_proxy, idx, mapped_node, cons_fn);

    auto elm_holder = findElmHolder<IndexType>(untyped_proxy);
    auto raw_ptr = elm_holder->lookup(idx).getRawPtr();
    raw_ptr->getStats().updatePhase(thePhase()->getCurrentPhase());
  } else if (insert_node != this_node) {
    auto msg = makeMessage<InsertMsg<ColT, MsgT>>(
      proxy, idx, insert_node, mapped_node, modify_epoch, insert_msg
    );
    theMsg()->markAsCollectionMessage(msg);
    theMsg()->sendMsg<InsertMsg<ColT, MsgT>,insertHandler<ColT, MsgT>>(
      insert_node, msg
    );
  }

  theMsg()->popEpoch(modify_epoch);
}

template <typename ColT>
void CollectionManager::destroyElm(
  CollectionProxyWrapType<ColT> const& proxy, typename ColT::IndexType idx,
  ModifierToken& token
) {
  using IndexType = typename ColT::IndexType;

  auto const modify_epoch = token.modifyEpoch();
  auto const untyped_proxy = proxy.getProxy();
  vtAssert(modify_epoch != no_epoch, "Modification epoch should be valid");

  vt_debug_print(
    normal, vrt_coll,
    "destroyElm: proxy={:x}\n",
    untyped_proxy
  );

  theMsg()->pushEpoch(modify_epoch);

  // First, check on this node for the element, and remove it if it's found
  auto elm_holder = findElmHolder<IndexType>(untyped_proxy);
  if (elm_holder->exists(idx)) {
    // Delay this so we can finish processing this work unit first (which might
    // be this collection element running)
    theSched()->enqueue([idx,untyped_proxy]{
      auto elm = theCollection()->findElmHolder<IndexType>(untyped_proxy);
      if (elm->exists(idx)) {
        elm->remove(idx);
      }
    });
  } else {
    // Otherwise, we send a destroy message that will be routed (eventually
    // arriving) where the element resides
    proxy(idx).template send<DestroyElmMsg<ColT>, destroyElmHandler<ColT>>(
      untyped_proxy, idx, modify_epoch
    );
  }

  theMsg()->popEpoch(modify_epoch);
}

template <typename ColT>
/*static*/ void CollectionManager::destroyElmHandler(
  DestroyElmMsg<ColT>* msg, ColT*
) {
  CollectionProxyWrapType<ColT> proxy{msg->proxy_};
  ModifierToken token{msg->modifier_epoch_};
  theCollection()->destroyElm(proxy, msg->idx_, token);
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
   terse, vrt_coll,
   "migrateOut: col_proxy={:x}, this_node={}, dest={}, "
   "idx={}\n",
   col_proxy, this_node, dest, print_index(idx)
 );

 if (this_node != dest) {
   auto const& proxy = CollectionProxy<ColT, IndexT>(col_proxy).operator()(
     idx
   );
   auto elm_holder = findElmHolder<IndexT>(col_proxy);
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
     verbose, vrt_coll,
     "migrateOut: (before remove) holder numElements={}\n",
     elm_holder->numElements()
   );

   if (elm_holder->numElements() == 1 and theConfig()->vt_lb_keep_last_elm) {
     vt_debug_print(
       normal, vrt_coll,
       "migrateOut: do not migrate last element\n"
     );
     return MigrateStatus::ElementNotLocal;
   }

   auto col_unique_ptr = elm_holder->remove(idx);
   auto& typed_col_ref = *static_cast<ColT*>(col_unique_ptr.get());

   vt_debug_print(
     verbose, vrt_coll,
     "migrateOut: (after remove) holder numElements={}\n",
     elm_holder->numElements()
   );

   /*
    * Invoke the virtual prelude migrate out function
    */
   col_unique_ptr->preMigrateOut();

   vt_debug_print(
     verbose, vrt_coll,
     "migrateOut: col_proxy={:x}, idx={}, dest={}: serializing collection elm\n",
     col_proxy, print_index(idx), dest
   );

   using MigrateMsgType = MigrateMsg<ColT, IndexT>;

   auto msg = makeMessage<MigrateMsgType>(proxy, this_node, dest, &typed_col_ref);

   theMsg()->sendMsg<
     MigrateMsgType, MigrateHandlers::migrateInHandler<ColT, IndexT>
   >(dest, msg);

   theLocMan()->getCollectionLM<IndexT>(col_proxy)->entityEmigrated(idx, dest);

   /*
    * Invoke the virtual epilog migrate out function
    */
   col_unique_ptr->epiMigrateOut();

   vt_debug_print(
     verbose, vrt_coll,
     "migrateOut: col_proxy={:x}, idx={}, dest={}: invoking destroy()\n",
     col_proxy, print_index(idx), dest
   );

   /*
    * Invoke the virtual destroy function and then null std::unique_ptr<ColT>,
    * which should cause the destructor to fire
    */
   col_unique_ptr->destroy();
   col_unique_ptr = nullptr;

   auto const home_node = getMappedNode<IndexT>(col_proxy, idx);
   elm_holder->applyListeners(
     listener::ElementEventEnum::ElementMigratedOut, idx, home_node
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
  VirtualPtrType<IndexT> vrt_elm_ptr
) {
  vt_debug_print(
    terse, vrt_coll,
    "CollectionManager::migrateIn: proxy={:x}, idx={}, from={}, ptr={}\n",
    proxy, print_index(idx), from, print_ptr(vrt_elm_ptr.get())
  );

  auto vc_raw_ptr = vrt_elm_ptr.get();

  /*
   * Invoke the virtual prelude migrate-in function
   */
  vc_raw_ptr->preMigrateIn();

  // Always update the element ID struct for LB statistic tracking
  auto const& this_node = theContext()->getNode();
  vrt_elm_ptr->elm_id_.curr_node = this_node;

  auto home_node = getMappedNode<ColT>(proxy, idx);
  auto const inserted = insertCollectionElement<ColT, IndexT>(
    std::move(vrt_elm_ptr), proxy, idx, home_node, true, from
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
  theMsg()->broadcastMsg<DestroyMsgType, DestroyHandlers::destroyNow>(msg);
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
    normal, vrt_coll,
    "destroyMatching: proxy={:x}\n",
    proxy.getProxy()
  );

  auto const untyped_proxy = proxy.getProxy();
  typeless_holder_.destroyCollection(untyped_proxy);
  auto elm_holder = findElmHolder<IndexT>(untyped_proxy);
  if (elm_holder) {
    elm_holder->foreach([&](IndexT const& idx, Indexable<IndexT>*) {
      auto const home = getMappedNode<IndexT>(untyped_proxy, idx);
      elm_holder->applyListeners(
        listener::ElementEventEnum::ElementDestroyed, idx, home
      );
    });
    elm_holder->destroyAll();
  }

  EntireHolder<IndexT>::remove(untyped_proxy);

  auto iter = cleanup_fns_.find(untyped_proxy);
  if (iter != cleanup_fns_.end()) {
    cleanup_fns_.erase(iter);
  }
}

template <typename IndexT>
CollectionHolder<IndexT>* CollectionManager::findColHolder(
  VirtualProxyType const& proxy
) {
  #pragma sst global proxy_container_
  auto& holder_container = EntireHolder<IndexT>::proxy_container_;
  auto holder_iter = holder_container.find(proxy);
  auto const& found_holder = holder_iter != holder_container.end();
  if (found_holder) {
    return holder_iter->second.get();
  } else {
    return nullptr;
  }
}

template <typename IndexT>
Holder<IndexT>* CollectionManager::findElmHolder(
  VirtualProxyType const& proxy
) {
  auto ret = findColHolder<IndexT>(proxy);
  if (ret != nullptr) {
    return &ret->holder_;
  } else {
    return nullptr;
  }
}

template <typename ProxyT, typename IndexT>
Holder<IndexT>* CollectionManager::findElmHolder(ProxyT proxy) {
  return findElmHolder<IndexT>(proxy.getProxy());
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
  auto elm_holder = findElmHolder<IndexT>(proxy);
  return elm_holder->addListener(fn);
}

template <typename ColT, typename IndexT>
void CollectionManager::unregisterElementListener(
  VirtualProxyType proxy, int element
) {
  auto elm_holder = findElmHolder<IndexT>(proxy);
  elm_holder->removeListener(element);
}

template <typename ColT, typename IndexT>
IndexT CollectionManager::getRange(VirtualProxyType proxy) {
  auto col_holder = findColHolder<IndexT>(proxy);
  return col_holder->bounds;
}

template <typename ColT, typename IndexT>
std::set<IndexT> CollectionManager::getLocalIndices(
  CollectionProxyWrapType<ColT> proxy
) {
  auto elm_holder = findElmHolder<IndexT>(proxy.getProxy());
  std::set<IndexT> local;
  elm_holder->foreach([&](IndexT const& idx, Indexable<IndexT>*) {
    local.insert(idx);
  });
  return local;
}

template <typename IndexT>
std::string CollectionManager::makeMetaFilename(
  std::string file_base, bool make_sub_dirs
) {
  auto this_node = theContext()->getNode();
  if (make_sub_dirs) {

    int flag = 0;
    flag = mkdir(file_base.c_str(), S_IRWXU);
    if (flag < 0 && errno != EEXIST) {
      throw std::runtime_error("Failed to create directory: " + file_base);
    }

    auto subdir = fmt::format("{}/directory-{}", file_base, this_node);
    flag = mkdir(subdir.c_str(), S_IRWXU);
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
    normal, vrt_coll,
    "checkpointToFile: proxy={:x}, file_base={}\n",
    proxy_bits, file_base
  );

  // Get the element holder
  auto holder_ = findElmHolder<IndexT>(proxy_bits);
  vtAssert(holder_ != nullptr, "Must have valid holder for collection");

  auto range = getRange<ColT>(proxy_bits);

  CollectionDirectory<IndexT> directory;

  holder_->foreach([&](IndexT const& idx, Indexable<IndexT>* elm) {
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

namespace detail {
template <typename ColT>
inline void restoreOffHomeElement(
  CollectionManager::RestoreMigrateColMsg<ColT>* msg, ColT*
) {
  auto idx = msg->idx_;
  auto node = msg->to_node_;
  auto proxy = msg->proxy_;
  theCollection()->migrate(proxy(idx), node);
}
} /* end namespace detail */

template <typename ColT>
/*static*/ void CollectionManager::migrateToRestoreLocation(
  RestoreMigrateMsg<ColT>* msg
) {
  auto idx = msg->idx_;
  auto node = msg->to_node_;
  auto proxy = msg->proxy_;
  if (proxy(idx).tryGetLocalPtr() != nullptr) {
    theCollection()->migrate(proxy(idx), node);
  } else {
    proxy(idx).template send<
      RestoreMigrateColMsg<ColT>, detail::restoreOffHomeElement<ColT>
    >(node, idx, proxy);
  }
}

template <typename ColT>
void CollectionManager::restoreFromFileInPlace(
  CollectionProxyWrapType<ColT> proxy, typename ColT::IndexType range,
  std::string const& file_base
) {
  using IndexType = typename ColT::IndexType;
  using DirectoryType = CollectionDirectory<IndexType>;

  auto proxy_bits = proxy.getProxy();

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

  runInEpochCollective([&]{
    for (auto&& elm : directory->elements_) {
      auto idx = elm.idx_;
      auto file_name = elm.file_name_;

      if (proxy(idx).tryGetLocalPtr() == nullptr) {
        auto mapped_node = getMappedNode<ColT>(proxy, idx);
        vtAssertExpr(mapped_node != uninitialized_destination);
        auto this_node = theContext()->getNode();

        using MsgType = RestoreMigrateMsg<ColT>;
        auto msg = makeMessage<MsgType>(this_node, idx, proxy);
        if (mapped_node != this_node) {
          theMsg()->sendMsg<MsgType, migrateToRestoreLocation<ColT>>(
            mapped_node, msg
          );
        } else {
          migrateToRestoreLocation<ColT>(msg.get());
        }
      }
    }
  });

  for (auto&& elm : directory->elements_) {
    auto idx = elm.idx_;
    auto file_name = elm.file_name_;
    vtAssertExpr(proxy(idx).tryGetLocalPtr() != nullptr);

    auto holder = findColHolder<IndexType>(proxy_bits);
    vtAssertExpr(holder != nullptr);

    auto elm_holder = findElmHolder<IndexType>(proxy_bits);
    auto const elm_exists = elm_holder->exists(idx);
    vtAssertExpr(elm_exists);

    auto ptr = elm_holder->lookup(idx).getRawPtr();
    checkpoint::deserializeInPlaceFromFile<ColT>(file_name, static_cast<ColT*>(ptr));
    ptr->stats_.resetPhase();
  }
}

template <typename ColT>
CollectionManager::CollectionProxyWrapType<ColT>
CollectionManager::restoreFromFile(
  typename ColT::IndexType range, std::string const& file_base
) {
  using IndexType = typename ColT::IndexType;
  using DirectoryType = CollectionDirectory<IndexType>;

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

  std::vector<std::tuple<IndexType, std::unique_ptr<ColT>>> elms;
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
    col_ptr->stats_.resetPhase();
    elms.emplace_back(std::make_tuple(idx, std::move(col_ptr)));
  }

  return vt::makeCollection<ColT>()
    .bounds(range)
    .collective(true)
    .listInsertHere(std::move(elms))
    .wait();
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

#include "vt/vrt/collection/collection_builder.impl.h"
#include "vt/vrt/collection/param/construct_params.impl.h"

#endif /*INCLUDED_VT_VRT_COLLECTION_MANAGER_IMPL_H*/
