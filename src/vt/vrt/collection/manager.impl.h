/*
//@HEADER
// ************************************************************************
//
//                          manager.impl.h
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
#include "vt/vrt/collection/dispatch/dispatch.h"
#include "vt/vrt/collection/dispatch/registry.h"
#include "vt/vrt/collection/holders/insert_context_holder.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/registry/auto/map/auto_registry_map.h"
#include "vt/registry/auto/view/auto_registry_view.h"
#include "vt/registry/auto/collection/auto_registry_collection.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/topos/mapping/mapping_headers.h"
#include "vt/termination/term_headers.h"
#include "vt/serialization/serialization.h"
#include "vt/serialization/auto_dispatch/dispatch.h"
#include "vt/collective/reduce/reduce_hash.h"
#include "vt/runnable/collection.h"
#include "manager.h"

#include <tuple>
#include <utility>
#include <functional>
#include <cassert>
#include <memory>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace vt { namespace vrt { namespace collection {

template <typename>
/*static*/ VirtualIDType CollectionManager::curIdent_ = 0;

template <typename ColT>
/*static*/ CollectionManager::BcastBufferType<ColT>
CollectionManager::broadcasts_ = {};

template <typename IndexT>
std::unordered_map<VirtualProxyType, IndexT>
/*static*/ CollectionManager::view_range_ = {};

template <typename>
void CollectionManager::cleanupAll() {
  /*
   *  Destroy all the current live collections
   */
  destroyCollections<>();
  /*
   *  Run the cleanup functions for type-specific cleanup that can not be
   *  performed without capturing the type of each collection
   */
  for (auto fn : cleanup_fns_) {
    fn();
  }
  cleanup_fns_.clear();
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

template <typename SysMsgT>
/*static*/ void CollectionManager::distConstruct(SysMsgT* msg) {
  using ColT        = typename SysMsgT::CollectionType;
  using IndexT      = typename SysMsgT::IndexType;
  using Args        = typename SysMsgT::ArgsTupleType;
  using BaseIdxType = vt::index::BaseIndex;

  static constexpr auto num_args = std::tuple_size<Args>::value;

  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  auto& info = msg->info;

  // The VirtualProxyType for this construction
  auto proxy = info.getProxy();
  // The insert epoch for this collection
  auto const& insert_epoch = info.getInsertEpoch();
  // Count the number of elements locally created here
  int64_t num_elements_created = 0;
  // Get the mapping function handle
  auto const& map_han = msg->map;
  // Get the range for the construction
  auto range = msg->info.range_;
  // Get the proxy/collection dimension
  auto const dim = range.ndims();

  // save proxy meta-data
  theCollection()->setParent(proxy, proxy);
  theCollection()->setDim(proxy, dim);
  theCollection()->setRange(proxy, range);

  theCollection()->insertCollectionInfo(proxy,msg->map,insert_epoch);

  if (info.immediate_) {
    // Get the handler function
    auto fn = auto_registry::getHandlerMap(map_han);
    // Total count across the statically sized collection
    auto const& num_elms = info.range_.getSize();

    debug_print(
      vrt_coll, node,
      "running foreach: size={}, range={}, map_han={}\n",
      num_elms, range, map_han
    );

    range.foreach([&](IndexT cur_idx) mutable {
      debug_print(
        verbose, vrt_coll, node,
        "running foreach: before map: cur_idx={}, range={}\n",
        cur_idx.toString(), range.toString()
      );

      auto const cur = static_cast<BaseIdxType*>(&cur_idx);
      auto const max = static_cast<BaseIdxType*>(&range);

      auto mapped_node = fn(cur, max, num_nodes);

      debug_print(
        vrt_coll, node,
        "construct: foreach: node={}, cur_idx={}, max_range={}\n",
        mapped_node, cur_idx.toString(), range.toString()
      );

      if (this_node == mapped_node) {
        using IdxContextHolder = InsertContextHolder<IndexT>;

        // Actually construct the element. If the detector is enabled, call the
        // detection-based overloads to invoke the constructor with the
        // optionally-positional index. Otherwise, invoke constructor without
        // the index as a parameter

        // Set the current context index to `cur_idx`. This enables the user to
        // query the index of their collection element in the constructor, which
        // is often very handy
        IdxContextHolder::set(&cur_idx,proxy);

        #if backend_check_enabled(detector)
          auto new_vc = DerefCons::derefTuple<ColT, IndexT, decltype(msg->tup)>(
            num_elms, cur_idx, &msg->tup
          );
        #else
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

  debug_print(
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
  debug_print(
    vrt_coll, node,
    "createGroupCollection: proxy={:x}, in_group={}\n",
    proxy, in_group
  );

  auto const& vid = VirtualProxyBuilder::getVirtualID(proxy);
  auto const group_id = theGroup()->newGroupCollective(
    in_group, [proxy,vid](GroupType new_group){
      auto const& group_root = theGroup()->groupRoot(new_group);
      auto const& is_group_default = theGroup()->groupDefault(new_group);
      auto const& in_group = theGroup()->inGroup(new_group);
      auto elm_holder = theCollection()->findElmHolder<ColT,IndexT>(proxy);
      elm_holder->setGroup(new_group);
      elm_holder->setUseGroup(!is_group_default);
      elm_holder->setGroupReady(true);
      if (!is_group_default) {
        elm_holder->setGroupRoot(group_root);
      }

      debug_print(
        vrt_coll, node,
        "group finished construction: proxy={:x}, new_group={:x}, use_group={}, "
        "ready={}, root={}, is_group_default={}\n",
        proxy, new_group, elm_holder->useGroup(), elm_holder->groupReady(),
        group_root, is_group_default
      );

      if (!is_group_default && in_group) {
        uint64_t const group_tag_mask = 0x0fff0000;
        auto group_msg = makeSharedMessage<CollectionGroupMsg>(proxy,new_group);
        auto const& group_tag_id = vid | group_tag_mask;
        debug_print(
          vrt_coll, node,
          "calling group (construct) reduce: proxy={:x}\n", proxy
        );
        theGroup()->groupReduce(new_group)->reduce<
          CollectionGroupMsg,
          collectionGroupReduceHan
        >(group_root, group_msg, group_tag_id);
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

  debug_print(
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
  NodeType from
) {
  auto& user_msg = msg->getMsg();
  auto user_msg_ptr = &user_msg;
  // Be careful with type casting here..convert to typeless before
  // reinterpreting the pointer so the compiler does not produce the wrong
  // offset
  void* raw_ptr = static_cast<void*>(base);
  auto ptr = reinterpret_cast<UntypedCollection*>(raw_ptr);

  // Expand out the index for tracing purposes; Projections takes up to
  // 4-dimensions
  auto const idx = base->getIndex();
  uint64_t const idx1 = idx.ndims() > 0 ? idx[0] : 0;
  uint64_t const idx2 = idx.ndims() > 1 ? idx[1] : 0;
  uint64_t const idx3 = idx.ndims() > 2 ? idx[2] : 0;
  uint64_t const idx4 = idx.ndims() > 3 ? idx[3] : 0;
  runnable::RunnableCollection<UserMsgT,UntypedCollection>::run(
    han, user_msg_ptr, ptr, from, member, idx1, idx2, idx3, idx4
  );
}

template <typename ColT, typename IndexT, typename MsgT, typename UserMsgT>
/*static*/ CollectionManager::IsNotWrapType<ColT,UserMsgT,MsgT>
CollectionManager::collectionAutoMsgDeliver(
  MsgT* msg, CollectionBase<ColT,IndexT>* base, HandlerType han, bool member,
  NodeType from
) {
  // Be careful with type casting here..convert to typeless before
  // reinterpreting the pointer so the compiler does not produce the wrong
  // offset
  void* raw_ptr = static_cast<void*>(base);
  auto ptr = reinterpret_cast<UntypedCollection*>(raw_ptr);

  // Expand out the index for tracing purposes; Projections takes up to
  // 4-dimensions
  auto const idx = base->getIndex();
  uint64_t const idx1 = idx.ndims() > 0 ? idx[0] : 0;
  uint64_t const idx2 = idx.ndims() > 1 ? idx[1] : 0;
  uint64_t const idx3 = idx.ndims() > 2 ? idx[2] : 0;
  uint64_t const idx4 = idx.ndims() > 3 ? idx[3] : 0;
  runnable::RunnableCollection<MsgT,UntypedCollection>::run(
    han, msg, ptr, from, member, idx1, idx2, idx3, idx4
  );
}

template <typename ColT, typename IndexT, typename MsgT>
/*static*/ void CollectionManager::collectionBcastHandler(MsgT* msg) {
  auto const col_msg = static_cast<CollectionMessage<ColT>*>(msg);
  auto const bcast_proxy = col_msg->getBcastProxy();
  auto const is_wrap = col_msg->getWrap();
  auto const& untyped_proxy = bcast_proxy;
  auto const& group = envelopeGetGroup(msg->env);
  auto const& cur_epoch = theMsg()->getEpoch();
  auto const& msg_epoch = envelopeGetEpoch(msg->env);
  theMsg()->pushEpoch(cur_epoch);
  debug_print(
    vrt_coll, node,
    "collectionBcastHandler: bcast_proxy={:x}, han={}, bcast epoch={}, "
    "epoch={}, msg epoch={}, group={}, default group={}\n",
    bcast_proxy, col_msg->getVrtHandler(), col_msg->getBcastEpoch(),
    cur_epoch, msg_epoch, group, default_group
  );
  auto elm_holder = theCollection()->findElmHolder<ColT,IndexT>(bcast_proxy);
  if (elm_holder) {

    auto const handler = col_msg->getVrtHandler();
    auto const member  = col_msg->getMember();
    auto const range   = col_msg->getRange();

    debug_print(
      vrt_coll, node,
      "broadcast apply: size={}\n", elm_holder->numElements()
    );

    elm_holder->foreach([col_msg, msg, handler, member, range](
      IndexT idx, CollectionBase<ColT,IndexT>* base
    ) {
      debug_print(
        vrt_coll, node,
        "broadcast: apply to element: idx={}, epoch={}, bcast_epoch={}\n",
        print_index(idx), msg->bcast_epoch_, base->cur_bcast_epoch_
      );
      if (base->cur_bcast_epoch_ == msg->bcast_epoch_ - 1) {
        vtAssert(base != nullptr, "Must be valid pointer");
        base->cur_bcast_epoch_++;

        backend_enable_if(
          lblite, {
            debug_print(
              vrt_coll, node,
              "broadcast: apply to element: instrument={}\n",
              msg->lbLiteInstrument()
            );

            if (msg->lbLiteInstrument()) {
              auto& stats = base->getStats();
              stats.startTime();
            }
          }
        );

        bool process_elem = true;

        // check if current proxy is actually a view
        // and apply filtering if so
        auto const view_proxy = msg->getViewProxy();

        if (view_proxy != no_vrt_proxy) {
          auto const view_han = msg->getViewHandler();
          vtAssert(view_han != uninitialized_handler, "Should be registered");

          auto const filter = auto_registry::getHandlerView(view_han);
          auto base_idx = static_cast<vt::index::BaseIndex*>(&idx);
          // nb: multi-dimensional
          process_elem = idx < range and filter(base_idx);
        }

        if (process_elem) {
          // be very careful here, do not touch `base' after running the active
          // message because it might have migrated out and be invalid
          auto const from = col_msg->getFromNode();
          collectionAutoMsgDeliver<ColT,IndexT,MsgT,typename MsgT::UserMsgType>(
            msg,base,handler,member,from
          );
        }

        backend_enable_if(
          lblite, {
            if (msg->lbLiteInstrument()) {
              auto& stats = base->getStats();
              stats.stopTime();
            }
          }
        );
      }
    });
  }
  /*
   *  Buffer the broadcast message for later delivery (elements that migrate
   *  in), inserted elements, etc.
   */
  auto col_holder = theCollection()->findColHolder<ColT,IndexT>(untyped_proxy);
  if (!col_holder->is_static_) {
    auto const& epoch = msg->bcast_epoch_;
    /*
     * @todo: buffer the broadcasts only when needed and clean up appropriately
     */
    // theCollection()->bufferBroadcastMsg<ColT>(untyped_proxy, epoch, msg);
  }
  /*
   *  Termination: consume for default epoch for correct termination: on the
   *  other end the sender produces p units for each broadcast to the default
   *  group
   */
  debug_print(
    vrt_coll, node,
    "collectionBcastHandler: (consume) bcast_proxy={:x}, han={}, bcast epoch={}, "
    "epoch={}, msg epoch={}, group={}, default group={}\n",
    bcast_proxy, col_msg->getVrtHandler(), col_msg->getBcastEpoch(),
    cur_epoch, msg_epoch, group, default_group
  );
  if (group == default_group) {
    theTerm()->consume(cur_epoch);
  }
  theMsg()->popEpoch();
}

template <typename ColT, typename MsgT>
void CollectionManager::bufferBroadcastMsg(
  VirtualProxyType const& proxy, EpochType const& epoch, MsgT* msg
) {
  auto proxy_iter = broadcasts_<ColT>.find(proxy);
  if (proxy_iter == broadcasts_<ColT>.end()) {
    if (broadcasts_<ColT>.size() == 0) {
      cleanup_fns_.push_back([]{ broadcasts_<ColT>.clear(); });
    }
    broadcasts_<ColT>.emplace(
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
  auto proxy_iter = broadcasts_<ColT>.find(proxy);
  if (proxy_iter != broadcasts_<ColT>.end()) {
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
  auto proxy_iter = broadcasts_<ColT>.find(proxy);
  if (proxy_iter != broadcasts_<ColT>.end()) {
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
  auto& buffered = theCollection()->buffered_group_;
  auto iter = buffered.find(proxy);
  debug_print(
    vrt_coll, node,
    "collectionGroupFinishedHan: proxy={:x}, buffered size={}, epoch:{}\n",
    proxy, (iter != buffered.end() ? iter->second.size() : 0), theMsg()->getEpoch()
  );
  if (iter != buffered.end()) {
    for (auto&& elm : iter->second) {
      elm(proxy);
    }
    iter->second.clear();
    buffered.erase(iter);
  }

//  auto elm_holder = theCollection()->findElmHolder<ColT,IndexT>(proxy);
//  elm_holder.setGroupReady(true);
}




template <typename>
/*static*/ void CollectionManager::collectionFinishedHan(
  CollectionConsMsg* msg
) {
  auto const& proxy = msg->proxy;
  theCollection()->constructed_.insert(proxy);
  auto& buffered = theCollection()->buffered_bcasts_;
  auto iter = buffered.find(proxy);
  debug_print(
    vrt_coll, node,
    "collectionFinishedHan: proxy={:x}, buffered size={}\n",
    proxy, (iter != buffered.end() ? iter->second.size() : 0)
  );
  if (iter != buffered.end()) {
    for (auto&& elm : iter->second) {
      elm(proxy);
    }
    iter->second.clear();
    buffered.erase(iter);
  }
}

template <typename>
/*static*/ void CollectionManager::collectionConstructHan(
  CollectionConsMsg* msg
) {
  debug_print(
    vrt_coll, node,
    "collectionConstructHan: proxy={:x}\n", msg->proxy
  );
  if (msg->isRoot()) {
    auto new_msg = makeMessage<CollectionConsMsg>(*msg);
    theMsg()->broadcastMsg<CollectionConsMsg,collectionFinishedHan>(
      new_msg.get()
    );
    collectionFinishedHan(new_msg.get());
  } else {
    // do nothing
  }
}


template <typename>
/*static*/ void CollectionManager::collectionGroupReduceHan(
  CollectionGroupMsg* msg
) {
  debug_print(
    vrt_coll, node,
    "collectionGroupReduceHan: proxy={:x}, root={}, group={}\n",
    msg->proxy, msg->isRoot(), msg->getGroup()
  );
  if (msg->isRoot()) {
    auto nmsg = makeSharedMessage<CollectionGroupMsg>(*msg);
    theMsg()->broadcastMsg<CollectionGroupMsg,collectionGroupFinishedHan>(nmsg);
  }
}

template <typename ColT, typename IndexT, typename MsgT>
/*static*/ void CollectionManager::collectionMsgTypedHandler(MsgT* msg) {
  auto const col_msg = static_cast<CollectionMessage<ColT>*>(msg);
  auto const entity_proxy = col_msg->getProxy();
  auto const cur_epoch = getCurrentEpoch(msg);
  auto const& col = entity_proxy.getCollectionProxy();
  auto const& elm = entity_proxy.getElementProxy();
  auto const& idx = elm.getIndex();
  auto elm_holder = theCollection()->findElmHolder<ColT, IndexT>(col);

  bool const& exists = elm_holder->exists(idx);

  debug_print(
    vrt_coll, node,
    "collectionMsgTypedHandler: exists={}, idx={}, cur_epoch={}\n",
    exists, idx, cur_epoch
  );

  vtAssertInfo(exists, "Proxy must exist", cur_epoch, idx);

  auto& inner_holder = elm_holder->lookup(idx);

  auto const sub_handler = col_msg->getVrtHandler();
  auto const member = col_msg->getMember();
  auto const col_ptr = inner_holder.getCollection();

  debug_print(
    vrt_coll, node,
    "collectionMsgTypedHandler: sub_handler={}\n", sub_handler
  );

  vtAssertInfo(
    col_ptr != nullptr, "Must be valid pointer",
    sub_handler, member, cur_epoch, idx, exists
  );

  backend_enable_if(
    lblite, {
      debug_print(
        vrt_coll, node,
        "collectionMsgTypedHandler: receive msg: instrument={}\n",
        col_msg->lbLiteInstrument()
      );
      if (col_msg->lbLiteInstrument()) {
        auto& stats = col_ptr->getStats();
        stats.startTime();
      }
    }
  );

  theMsg()->pushEpoch(cur_epoch);
  auto const from = col_msg->getFromNode();
  collectionAutoMsgDeliver<ColT,IndexT,MsgT,typename MsgT::UserMsgType>(
    msg,col_ptr,sub_handler,member,from
  );
  theMsg()->popEpoch();

  backend_enable_if(
    lblite, {
      if (col_msg->lbLiteInstrument()) {
        auto& stats = col_ptr->getStats();
        stats.stopTime();
      }
    }
  );

  theTerm()->consume(cur_epoch);
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
void CollectionManager::broadcastFromRoot(MsgT* raw_msg) {
  auto msg = promoteMsg(raw_msg);

  // broadcast to all nodes
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& proxy = msg->getBcastProxy();
  auto elm_holder = theCollection()->findElmHolder<ColT,IndexT>(proxy);
  auto const bcast_node = VirtualProxyBuilder::getVirtualNode(proxy);

  vtAssert(elm_holder != nullptr, "Must have elm holder");
  vtAssert(this_node == bcast_node, "Must be the bcast node");

  auto const bcast_epoch = elm_holder->cur_bcast_epoch_++;
  auto const cur_epoch = getCurrentEpoch(msg.get());
  theMsg()->pushEpoch(cur_epoch);

  msg->setBcastEpoch(bcast_epoch);

  debug_print(
    vrt_coll, node,
    "broadcastFromRoot: proxy={:x}, epoch={}, han={}\n",
    proxy, msg->getBcastEpoch(), msg->getVrtHandler()
  );

  auto const& group_ready = elm_holder->groupReady();
  auto const& use_group = elm_holder->useGroup();
  bool const send_group = group_ready && use_group;

  debug_print(
    vrt_coll, node,
    "broadcastFromRoot: proxy={:x}, bcast epoch={}, han={}, group_ready={}, "
    "group_active={}, use_group={}, send_group={}, group={:x}, cur_epoch={}\n",
    proxy, msg->getBcastEpoch(), msg->getVrtHandler(),
    group_ready, send_group, use_group, send_group,
    use_group ? elm_holder->group() : default_group, cur_epoch
  );

  if (send_group) {
    auto const& group = elm_holder->group();
    envelopeSetGroup(msg->env, group);
  } else {
    theTerm()->produce(cur_epoch, num_nodes);
  }

  theMsg()->broadcastMsgAuto<MsgT,collectionBcastHandler<ColT,IndexT>>(
    msg.get()
  );
  if (!send_group) {
    collectionBcastHandler<ColT,IndexT,MsgT>(msg.get());
  }

  theMsg()->popEpoch();
}

template <
  typename MsgT,
  ActiveColMemberTypedFnType<MsgT,typename MsgT::CollectionType> f
>
void CollectionManager::broadcastMsg(
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
void CollectionManager::broadcastMsg(
  CollectionProxyWrapType<typename MsgT::CollectionType> const& proxy,
  MsgT *msg, bool instrument
) {
  using ColT = typename MsgT::CollectionType;
  return broadcastMsg<MsgT,ColT,f>(proxy,msg,instrument);
}

template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
CollectionManager::IsColMsgType<MsgT>
CollectionManager::broadcastMsg(
  CollectionProxyWrapType<ColT> const& proxy, MsgT *msg, bool instrument
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
void CollectionManager::broadcastMsgImpl(
  CollectionProxyWrapType<ColT> const& proxy, MsgT *const msg, bool inst
) {
  // register the user's handler
  auto const& h = auto_registry::makeAutoHandlerCollectionMem<ColT,MsgT,f>(msg);
  return broadcastMsgUntypedHandler<MsgT>(proxy,msg,h,true,inst);
}

template <typename MsgT, typename ColT, ActiveColTypedFnType<MsgT,ColT> *f>
void CollectionManager::broadcastMsgImpl(
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
void CollectionManager::broadcastNormalMsg(
  CollectionProxyWrapType<ColT> const& proxy, MsgT *msg,
  HandlerType const& handler, bool const member,
  bool instrument
) {
  auto wrap_msg = makeSharedMessage<ColMsgWrap<ColT,MsgT>>(*msg);
  return broadcastMsgUntypedHandler<ColMsgWrap<ColT,MsgT>,ColT>(
    proxy, wrap_msg, handler, member, instrument
  );
}

template <typename MsgT>
/*static*/ EpochType CollectionManager::getCurrentEpoch(MsgT* msg) {
  auto const& any_epoch = term::any_epoch_sentinel;
  auto const& msg_epoch = envelopeGetEpoch(msg->env);

  // Prefer the epoch on the msg before the current handler epoch
  if (msg_epoch != no_epoch and msg_epoch != any_epoch) {
    // It has a valid non-global epoch, which should be used for tracking it's
    // causality
    return msg_epoch;
  } else {
    // Otherwise, use the active messenger's current epoch on the stack
    auto const& epoch_state = theMsg()->getEpoch();
    vtAssert(
      epoch_state != no_epoch, "Must have a valid epoch here",
      print_ptr(msg), epoch_state, msg_epoch, no_epoch, any_epoch
    );
    return epoch_state;
  }
}

template <typename MsgT, typename ColT, typename IdxT>
void CollectionManager::broadcastMsgUntypedHandler(
  CollectionProxyWrapType<ColT, IdxT> const& toProxy, MsgT *raw_msg,
  HandlerType const& handler, bool const member, bool instrument
) {

  auto const idx = makeVrtDispatch<MsgT,ColT>();
  auto const this_node = theContext()->getNode();
  auto msg = promoteMsg(raw_msg);

  debug_print(
    vrt_coll, node,
    "broadcastMsgUntypedHandler: msg={}, idx={}\n",
    print_ptr(raw_msg), idx
  );

  backend_enable_if(
    lblite,
    msg->setLBLiteInstrument(instrument);
  );

  // update to handle view case
  auto const cur_proxy = toProxy.getProxy();
  auto const cur_range = toProxy.getRange();
  bool const is_view   = VirtualProxyBuilder::isView(cur_proxy);
  auto const view_han  = getViewHandler(cur_proxy);
  bool ready = false;

  do {
    runScheduler();
    ready = not is_view or isViewReady(cur_proxy);
  } while (not ready);

  // @todo: implement the action `act' after the routing is finished
  auto const col_proxy = view_parent_[cur_proxy];
  auto holder = findColHolder<ColT,IdxT>(col_proxy);
  bool already_built = constructed_.find(col_proxy) != constructed_.end();
  ready &= (holder != nullptr) and already_built;

  auto const& cur_epoch = getCurrentEpoch(msg.get());

  auto msg_epoch = envelopeGetEpoch(msg->env);
  if (msg_epoch == no_epoch) {
    envelopeSetEpoch(msg->env, cur_epoch);
  }

  debug_print(
    vrt_coll, node,
    "broadcastMsgUntypedHandler: "
    "col_proxy={:x}, cur_proxy={:x}, cur_epoch={:x}, ready={}, is_view={}\n",
    col_proxy, cur_proxy, cur_epoch, ready, is_view
  );

  if (ready) {
    // save the user's handler in the message
    msg->setVrtHandler(handler);
    msg->setBcastProxy(col_proxy);
    msg->setFromNode(this_node);
    msg->setMember(member);

    if (is_view) {
      msg->setViewFlag(true);
      msg->setViewProxy(cur_proxy);
      msg->setViewHandler(view_han);
      msg->setRange(cur_range);
    }

    auto const bnode = VirtualProxyBuilder::getVirtualNode(col_proxy);

    if (this_node != bnode) {
      debug_print(
        vrt_coll, node,
        "broadcastMsgUntypedHandler: col_proxy={:x}, sending to root node={}, "
        "handler={}, cur_epoch={}\n",
        col_proxy, bnode, handler, cur_epoch
      );

      theMsg()->sendMsgAuto<MsgT,broadcastRootHandler<ColT,IdxT>>(
        bnode,msg.get()
      );
    } else {
      debug_print(
        vrt_coll, node,
        "broadcasting msg to collection: msg={}, handler={}\n",
        print_ptr(raw_msg), handler
      );
      broadcastFromRoot<ColT,IdxT,MsgT>(msg.get());
    }
  } else {
    auto iter = buffered_bcasts_.find(cur_proxy);
    if (iter == buffered_bcasts_.end()) {
      buffered_bcasts_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(cur_proxy),
        std::forward_as_tuple(ActionContainerType{})
      );
      iter = buffered_bcasts_.find(col_proxy);
    }
    vtAssert(iter != buffered_bcasts_.end(), "Must exist");

    debug_print(
      vrt_coll, node,
      "broadcastMsgUntypedHandler: pushing into buffered sends: cur_proxy={:x}\n",
      cur_proxy
    );

    theTerm()->produce(cur_epoch);

    debug_print(
      vrt_coll, node,
      "broadcastMsgUntypedHandler: cur_proxy={:x}, cur_epoch={}, buffering\n",
      cur_proxy, cur_epoch
    );

    iter->second.push_back([=](VirtualProxyType /*ignored*/){
      debug_print(
        vrt_coll, node,
        "broadcastMsgUntypedHandler: cur_proxy={:x}, running buffered\n",
        cur_proxy
      );
      theMsg()->pushEpoch(cur_epoch);
      theCollection()->broadcastMsgUntypedHandler<MsgT,ColT,IdxT>(
        toProxy, msg.get(), handler, member, instrument
      );
      theMsg()->popEpoch();
      theTerm()->consume(cur_epoch);
    });
  }
}

template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType CollectionManager::reduceMsgExpr(
  CollectionProxyWrapType<ColT, typename ColT::IndexType> const& toProxy,
  MsgT *const raw_msg, ReduceIdxFuncType<typename ColT::IndexType> expr_fn,
  EpochType const& epoch, TagType const& tag, NodeType const& root
) {
  using IndexT = typename ColT::IndexType;

  auto msg = promoteMsg(raw_msg);

  debug_print(
    vrt_coll, node,
    "reduceMsg: msg={}\n", print_ptr(raw_msg)
  );

  auto const& col_proxy = toProxy.getProxy();

  // @todo: implement the action `act' after the routing is finished
  auto found_constructed = constructed_.find(col_proxy) != constructed_.end();
  auto elm_holder = findElmHolder<ColT,IndexT>(col_proxy);

  auto const& group_ready = elm_holder->groupReady();
  auto const& send_group = elm_holder->useGroup();
  auto const& group = elm_holder->group();
  bool const use_group = group_ready && send_group;

  debug_print(
    vrt_coll, node,
    "reduceMsg: col_proxy={:x}, found={}, group={:x}, group_ready={}, "
    "use_group={}\n",
    col_proxy, found_constructed, group, group_ready, send_group
  );

  if (!group_ready) {
    auto iter = buffered_group_.find(col_proxy);
    if (iter == buffered_group_.end()) {
      buffered_group_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(col_proxy),
        std::forward_as_tuple(ActionContainerType{})
      );
      iter = buffered_group_.find(col_proxy);
    }
    vtAssert(iter != buffered_group_.end(), "Must exist");

    theTerm()->produce(term::any_epoch_sentinel);

    debug_print(
      vrt_coll, node,
      "reduceMsgExpr: col_proxy={:x}, buffering reduce operation\n",
      col_proxy
    );

    iter->second.push_back([=](VirtualProxyType /*ignored*/){
      debug_print(
        vrt_coll, node,
        "reduceMsgExpr: col_proxy={:x}, running buffered reduce operation\n",
        col_proxy
      );
      theTerm()->consume(term::any_epoch_sentinel);
      theCollection()->reduceMsgExpr<ColT,MsgT,f>(
        toProxy,msg.get(),expr_fn,epoch,tag,root
      );
    });

    return no_epoch;
  } else if (found_constructed && elm_holder) {
    std::size_t num_elms = 0;

    if (expr_fn == nullptr) {
      num_elms = elm_holder->numElements();
    } else {
      num_elms = elm_holder->numElementsExpr(expr_fn);
    }

    auto reduce_id = std::make_tuple(col_proxy,tag);
    auto epoch_iter = reduce_cur_epoch_.find(reduce_id);
    EpochType cur_epoch = epoch;
    if (epoch == no_epoch && epoch_iter != reduce_cur_epoch_.end()) {
      cur_epoch = epoch_iter->second;
    }
    EpochType ret_epoch = no_epoch;

    auto const& root_node =
      root == uninitialized_destination ? default_collection_reduce_root_node :
      root;

    if (use_group) {
      ret_epoch = theGroup()->groupReduce(group)->template reduce<MsgT,f>(
        root_node,msg.get(),tag,cur_epoch,num_elms,col_proxy
      );
    } else {
      ret_epoch = theCollective()->reduce<MsgT,f>(
        root_node,msg.get(),tag,cur_epoch,num_elms,col_proxy
      );
    }
    debug_print(
      vrt_coll, node,
      "reduceMsg: col_proxy={:x}, epoch={}, num_elms={}, tag={}\n",
      col_proxy, cur_epoch, num_elms, tag
    );
    if (epoch_iter == reduce_cur_epoch_.end()) {
      reduce_cur_epoch_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(reduce_id),
        std::forward_as_tuple(ret_epoch)
      );
    }

    return ret_epoch;
  } else {
    // @todo: implement this
    vtAssertExpr(0);
    return no_epoch;
  }
}

template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType CollectionManager::reduceMsg(
  CollectionProxyWrapType<ColT, typename ColT::IndexType> const& toProxy,
  MsgT *const msg, EpochType const& epoch, TagType const& tag,
  NodeType const& root
) {
  return reduceMsgExpr<ColT,MsgT,f>(toProxy,msg,nullptr,epoch,tag,root);
}

template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType CollectionManager::reduceMsg(
  CollectionProxyWrapType<ColT, typename ColT::IndexType> const& toProxy,
  MsgT *const msg, EpochType const& epoch, TagType const& tag,
  typename ColT::IndexType const& idx
) {
  return reduceMsgExpr<ColT,MsgT,f>(toProxy,msg,nullptr,epoch,tag,idx);
}

template <typename ColT, typename MsgT, ActiveTypedFnType<MsgT> *f>
EpochType CollectionManager::reduceMsgExpr(
  CollectionProxyWrapType<ColT, typename ColT::IndexType> const& toProxy,
  MsgT *const msg, ReduceIdxFuncType<typename ColT::IndexType> expr_fn,
  EpochType const& epoch, TagType const& tag,
  typename ColT::IndexType const& idx
) {
  using IndexT = typename ColT::IndexType;
  auto const untyped_proxy = toProxy.getProxy();
  auto constructed = constructed_.find(untyped_proxy) != constructed_.end();
  vtAssert(constructed, "Must be constructed");
  auto col_holder = findColHolder<ColT,IndexT>(untyped_proxy);
  auto max_idx = col_holder->max_idx;
  auto const& this_node = theContext()->getNode();
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
  return reduceMsgExpr<ColT,MsgT,f>(toProxy,msg,nullptr,epoch,tag,mapped_node);
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
void CollectionManager::sendNormalMsg(
  VirtualElmProxyType<ColT> const& proxy, MsgT *msg,
  HandlerType const& handler, bool const member
) {
  auto wrap_msg = makeSharedMessage<ColMsgWrap<ColT,MsgT>>(*msg);
  return sendMsgUntypedHandler<ColMsgWrap<ColT,MsgT>,ColT>(
    proxy, wrap_msg, handler, member
  );
}

template <
  typename MsgT, ActiveColTypedFnType<MsgT,typename MsgT::CollectionType> *f
>
void CollectionManager::sendMsg(
  VirtualElmProxyType<typename MsgT::CollectionType> const& proxy, MsgT *msg
) {
  using ColT = typename MsgT::CollectionType;
  return sendMsg<MsgT,ColT,f>(proxy,msg);
}

template <
  typename MsgT,
  ActiveColMemberTypedFnType<MsgT,typename MsgT::CollectionType> f
>
void CollectionManager::sendMsg(
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
void CollectionManager::sendMsgImpl(
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
void CollectionManager::sendMsgImpl(
  VirtualElmProxyType<ColT> const& proxy, MsgT *msg
) {
  auto const& h = auto_registry::makeAutoHandlerCollectionMem<ColT,MsgT,f>(msg);
  return sendMsgUntypedHandler<MsgT>(proxy,msg,h,true);
}

template <typename MsgT, typename ColT, typename IdxT>
void CollectionManager::sendMsgUntypedHandler(
  VirtualElmProxyType<ColT> const& toProxy, MsgT *raw_msg,
  HandlerType const& handler, bool const member,
  bool imm_context
) {
  // @todo: implement the action `action' after the routing is finished

  auto const& col_proxy = toProxy.getCollectionProxy();
  auto const& elm_proxy = toProxy.getElementProxy();

  auto msg = promoteMsg(raw_msg);

  backend_enable_if(
    lblite,
    msg->setLBLiteInstrument(true);
  );

  auto const& cur_epoch = getCurrentEpoch(msg.get());

  auto msg_epoch = envelopeGetEpoch(msg->env);
  if (msg_epoch == no_epoch) {
    envelopeSetEpoch(msg->env, cur_epoch);
  }

  debug_print(
    vrt_coll, node,
    "sendMsgUntypedHandler: col_proxy={:x}, cur_epoch={}, imm_context={}\n",
    col_proxy, cur_epoch, imm_context
  );

  if (imm_context) {
    theTerm()->produce(cur_epoch);
    schedule<>([=]{
      theMsg()->pushEpoch(cur_epoch);
      theCollection()->sendMsgUntypedHandler<MsgT,ColT,IdxT>(
        toProxy, msg.get(), handler, member, false
      );
      theMsg()->popEpoch();
      theTerm()->consume(cur_epoch);
    });
    return;
  } else {
    theTerm()->produce(cur_epoch);
  }

  // auto found_constructed = constructed_.find(col_proxy) != constructed_.end();

  auto holder = findColHolder<ColT, IdxT>(col_proxy);
  if (holder != nullptr /* && found_constructed*/) {
    theMsg()->pushEpoch(cur_epoch);

    auto const map_han = holder->map_fn;
    auto max_idx = holder->max_idx;
    auto cur_idx = elm_proxy.getIndex();
    auto fn = auto_registry::getAutoHandlerMap(map_han);

    auto const& num_nodes = theContext()->getNumNodes();

    auto home_node = fn(
      reinterpret_cast<vt::index::BaseIndex*>(&cur_idx),
      reinterpret_cast<vt::index::BaseIndex*>(&max_idx),
      num_nodes
    );

    msg->setVrtHandler(handler);
    msg->setProxy(toProxy);
    msg->setMember(member);

    debug_print(
      vrt_coll, node,
      "sending msg to collection: msg={}, handler={}, home_node={}\n",
      print_ptr(raw_msg), handler, home_node
    );

    // route the message to the destination using the location manager
    auto lm = theLocMan()->getCollectionLM<ColT, IdxT>(col_proxy);
    vtAssert(lm != nullptr, "LM must exist");
    lm->template routeMsgSerializeHandler<
      MsgT, collectionMsgTypedHandler<ColT,IdxT,MsgT>
    >(toProxy, home_node, msg);

    theMsg()->popEpoch();
  } else {
    auto iter = buffered_sends_.find(toProxy.getCollectionProxy());
    if (iter == buffered_sends_.end()) {
      buffered_sends_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(toProxy.getCollectionProxy()),
        std::forward_as_tuple(ActionContainerType{})
      );
      iter = buffered_sends_.find(toProxy.getCollectionProxy());
    }
    vtAssert(iter != buffered_sends_.end(), "Must exist");

    debug_print(
      vrt_coll, node,
      "sendMsgUntypedHandler: pushing into buffered sends: col_proxy={:x}\n",
      toProxy.getCollectionProxy()
    );

    theTerm()->produce(cur_epoch);

    iter->second.push_back([=](VirtualProxyType /*ignored*/){
      theMsg()->pushEpoch(cur_epoch);
      theCollection()->sendMsgUntypedHandler<MsgT,ColT,IdxT>(
        toProxy, msg.get(), handler, member, false
      );
      theMsg()->popEpoch();
      theTerm()->consume(cur_epoch);
    });
  }
}

template <typename ColT, typename IndexT>
bool CollectionManager::insertCollectionElement(
  VirtualPtrType<ColT, IndexT> vc, IndexT const& idx, IndexT const& max_idx,
  HandlerType const& map_han, VirtualProxyType const& proxy,
  bool const is_static, NodeType const& home_node, bool const& is_migrated_in,
  NodeType const& migrated_from
) {
  auto holder = findColHolder<ColT, IndexT>(proxy);

  debug_print(
    vrt_coll, node,
    "insertCollectionElement: proxy={:x}, map_han={}, idx={}, max_idx={}\n",
    proxy, map_han, print_index(idx), print_index(max_idx)
  );

  if (holder == nullptr) {
    using HolderType = typename EntireHolder<ColT, IndexT>::InnerHolder;

    EntireHolder<ColT, IndexT>::insert(
      proxy, std::make_shared<HolderType>(map_han,max_idx,is_static)
    );

    debug_print(
      vrt_coll, node,
      "looking for buffered sends: proxy={:x}, size={}\n",
      proxy, buffered_sends_.size()
    );

    auto iter = buffered_sends_.find(proxy);
    if (iter != buffered_sends_.end()) {
      debug_print(
        vrt_coll, node,
        "looking for buffered sends: FOUND\n"
      );

      for (auto&& elm : iter->second) {
        debug_print(
          vrt_coll, node,
          "looking for buffered sends: running elm\n"
        );

        elm(proxy);
      }
      iter->second.clear();
      buffered_sends_.erase(iter);
    }
  }

  auto elm_holder = findElmHolder<ColT,IndexT>(proxy);
  auto const& elm_exists = elm_holder->exists(idx);

  if (elm_exists) {
    return false;
  }
  vtAssert(!elm_exists, "Must not exist at this point");

  auto const& destroyed = elm_holder->isDestroyed();

  if (!destroyed) {
    elm_holder->insert(idx, typename Holder<ColT, IndexT>::InnerHolder{
      std::move(vc), map_han, max_idx
    });

    if (is_migrated_in) {
      theLocMan()->getCollectionLM<ColT, IndexT>(proxy)->registerEntityMigrated(
        VrtElmProxy<ColT, IndexT>{proxy,idx}, migrated_from,
        CollectionManager::collectionMsgHandler<ColT, IndexT>
      );
    } else {
      theLocMan()->getCollectionLM<ColT, IndexT>(proxy)->registerEntity(
        VrtElmProxy<ColT, IndexT>{proxy,idx}, home_node,
        CollectionManager::collectionMsgHandler<ColT, IndexT>
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

  auto const this_node = theContext()->getNode();
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
  auto loc = theLocMan()->getCollectionLM<ColT, IndexT>(proxy);

  debug_print(
    vrt_coll, node,
    "construct (dist): proxy={:x}, is_static={}\n",
    proxy, is_static
  );

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

    debug_print(
      verbose, vrt_coll, node,
      "construct (dist): foreach: map: cur_idx={}, index range={}\n",
      cur_idx.toString(), range.toString()
    );

    auto const cur = static_cast<BaseIdxType*>(&cur_idx);
    auto const max = static_cast<BaseIdxType*>(&range);

    auto mapped_node = fn(cur, max, num_nodes);

    debug_print(
      verbose, vrt_coll, node,
      "construct (dist): foreach: cur_idx={}, mapped_node={}\n",
      cur_idx.toString(), mapped_node
    );

    if (this_node == mapped_node) {
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

      debug_print(
        verbose, vrt_coll, node,
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

      debug_print(
        /*verbose, */vrt_coll, node,
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

  debug_print(
    vrt_coll, node,
    "constructCollectiveMap: entering wait for constructed_\n"
  );

  // Wait for construction to finish before we release control to the user; this
  // ensures that other parts of the system do not migrate elements until the
  // group construction is complete
  while (constructed_.find(proxy) == constructed_.end()) {
    vt::runScheduler();
  }

  debug_print(
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

  debug_print(
    vrt_coll, node,
    "makeDistProxy: node={}, new_dist_id={}, proxy={:x}\n",
    this_node, new_dist_id, proxy
  );

  return proxy;
}

/* end SPMD distributed collection support */


template <typename ColT, typename... Args>
void CollectionManager::staticInsert(
  VirtualProxyType proxy, typename ColT::IndexType idx, Args&&... args
) {
  using IndexT           = typename ColT::IndexType;
  using IdxContextHolder = InsertContextHolder<IndexT>;
  using BaseIdxType      = vt::index::BaseIndex;

  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  auto map_han = UniversalIndexHolder<>::getMap(proxy);

  // Set the current context index to `idx`
  IdxContextHolder::set(&idx,proxy);

  auto tuple = std::make_tuple(std::forward<Args>(args)...);

  static constexpr auto num_args = std::tuple_size<decltype(tuple)>::value;

  auto holder = findColHolder<ColT, IndexT>(proxy);

  auto range = holder->max_idx;
  auto const num_elms = range.getSize();

  // Get the handler function
  auto fn = auto_registry::getHandlerMap(map_han);

  auto const cur = static_cast<BaseIdxType*>(&idx);
  auto const max = static_cast<BaseIdxType*>(&range);
  auto const& home_node = fn(cur, max, num_nodes);

  #if backend_check_enabled(detector)
    auto elm_ptr = DerefCons::derefTuple<ColT, IndexT, decltype(tuple)>(
      num_elms, idx, &tuple
    );
  #else
    auto elm_ptr = CollectionManager::runConstructor<ColT, IndexT>(
      num_elms, idx, &tuple, std::make_index_sequence<num_args>{}
    );
  #endif

  debug_print(
    verbose, vrt_coll, node,
    "construct (staticInsert): ptr={}\n", print_ptr(elm_ptr.get())
  );

  // Through the attorney, setup all the properties on the newly constructed
  // collection element: index, proxy, number of elements. Note: because of
  // how the constructor works, the index is not currently available through
  // "getIndex"
  CollectionTypeAttorney::setup(elm_ptr.get(), num_elms, idx, proxy);

  // Insert the element into the managed holder for elements
  insertCollectionElement<ColT>(
    std::move(elm_ptr), idx, range, map_han, proxy, true, home_node
  );

  // Clear the current index context
  IdxContextHolder::clear();

}

template <typename ColT>
InsertToken<ColT> CollectionManager::constructInsert(
  typename ColT::IndexType range, TagType const& tag
) {
  using IndexT         = typename ColT::IndexType;

  auto const map_han = getDefaultMap<ColT>();

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
  auto loc = theLocMan()->getCollectionLM<ColT, IndexT>(proxy);

  // Start the local collection initiation process, lcoal meta-info about the
  // collection. Insert epoch is `no_epoch` because dynamic insertions are not
  // allowed when using SPMD distributed construction currently
  insertCollectionInfo(proxy, map_han, no_epoch);

  // Insert the meta-data for this new collection
  insertMetaCollection<ColT>(proxy, map_han, range, is_static);

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
  auto const idx = IdxContextHolder::index();
  vtAssertExpr(idx != nullptr);
  return idx;
}

template <typename IndexT>
/*static*/ VirtualProxyType CollectionManager::queryProxyContext() {
  using IdxContextHolder = InsertContextHolder<IndexT>;
  auto const proxy = IdxContextHolder::proxy();
  vtAssertExpr(proxy != no_vrt_proxy);
  return proxy;
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
  uint64_t const tag_mask_ = 0x0ff00000;
  auto const& vid = VirtualProxyBuilder::getVirtualID(proxy);
  auto construct_msg = makeSharedMessage<CollectionConsMsg>(proxy);
  auto const& tag_id = vid | tag_mask_;
  auto const& root = 0;
  debug_print(
    vrt_coll, node,
    "reduceConstruction: invoke reduce: proxy={:x}\n", proxy
  );
  theCollective()->reduce<CollectionConsMsg,collectionConstructHan>(
    root, construct_msg, tag_id
  );
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

    debug_print(
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
  using SerdesMsg = SerializedMessenger;
  using IndexT = typename ColT::IndexType;
  using ArgsTupleType = std::tuple<typename std::decay<Args>::type...>;
  using MsgType = CollectionCreateMsg<
    CollectionInfo<ColT, IndexT>, ArgsTupleType, ColT, IndexT
  >;

  auto const& new_proxy = makeNewCollectionProxy();
  auto const& is_static = ColT::isStaticSized();
  auto lm = theLocMan()->getCollectionLM<ColT, IndexT>(new_proxy);
  auto const& node = theContext()->getNode();
  auto create_msg = makeMessage<MsgType>(
    map_handler, ArgsTupleType{std::forward<Args>(args)...}
  );

  CollectionInfo<ColT, IndexT> info(range, is_static, node, new_proxy);

  if (!is_static) {
    auto const& insert_epoch = theTerm()->newEpochRooted(false,false);
    info.setInsertEpoch(insert_epoch);
    setupNextInsertTrigger<ColT,IndexT>(new_proxy,insert_epoch);
  }

  create_msg->info = info;

  debug_print(
    vrt_coll, node,
    "construct_map: range={}\n", range.toString().c_str()
  );

  SerdesMsg::broadcastSerialMsg<MsgType,distConstruct<MsgType>>(
    create_msg.get()
  );

  auto create_msg_local = makeMessage<MsgType>(
    map_handler, ArgsTupleType{std::forward<Args>(args)...}
  );
  create_msg_local->info = info;
  CollectionManager::distConstruct<MsgType>(create_msg_local.get());

  return CollectionProxyWrapType<ColT, IndexT>{new_proxy, range};
}


template <
  typename ColT, typename IndexT,
  mapping::ActiveViewTypedFnType<IndexT>* filter
>
CollectionProxy<ColT, IndexT> CollectionManager::slice(
  CollectionProxy<ColT, IndexT> const& col_proxy,
  IndexT const& old_range,
  IndexT const& new_range,
  EpochType const& epoch,
  TagType const& tag
) {
  // message type for view group creation
  using MsgT = ViewCreateMsg<ColT, IndexT, IndexT>;

  // check that the collection is static and already built
  auto const old_proxy  = col_proxy.getProxy();
  auto const elm_holder = theCollection()->findElmHolder<ColT,IndexT>(old_proxy);

  bool group_ready = elm_holder->groupReady();
  bool no_pending = (buffered_group_.find(old_proxy) == buffered_group_.end());
  bool finished = group_ready and no_pending;

  while (not finished) {
    // spin until collection group is ready
    vt::runScheduler();
    group_ready = elm_holder->groupReady();
    no_pending = (buffered_group_.find(old_proxy) == buffered_group_.end());
    finished = group_ready and no_pending;
  }

  bool const is_static  = ColT::isStaticSized();
  bool const is_already_built  = (constructed_.find(old_proxy) != constructed_.end());
  bool const is_view_old_proxy = VirtualProxyBuilder::isView(old_proxy);

  vtAssert(is_static, "Only view of static collections are managed");
  vtAssert(is_already_built, "Collection should be already built");
  vtAssert(not is_view_old_proxy, "View of a view are not allowed for now");

  // register the user defined filtering function, so it can be invoked on other nodes
  auto const new_view_han = auto_registry::makeAutoHandlerView<IndexT,filter>();
  auto const old_view_han = uninitialized_handler;
  auto const mapping_id   = UniversalIndexHolder<>::getMap(old_proxy);

  // create a new proxy
  auto const new_proxy = makeNewCollectionProxy(true);
  bool const is_view_new_proxy = VirtualProxyBuilder::isView(new_proxy);
  vtAssertExpr(is_view_new_proxy);

  // save handler type for further queries
  saveViewHandler(new_proxy, new_view_han);

  // set some flags
  theCollection()->setViewReady(false);

  // broadcast view group creation request
  auto msg = makeSharedMessage<MsgT>(
    old_proxy, new_proxy,
    old_range, new_range,
    old_view_han, new_view_han, mapping_id,
    epoch, tag
  );

  SerializedMessenger::broadcastSerialMsg<MsgT, createViewGroup<MsgT>>(msg);

  // apply it locally
  createViewGroup<MsgT>(msg);

  // create the real new proxy
  return CollectionProxy<ColT, IndexT>{new_proxy, new_range};
}

template <typename SysMsgT>
/*static*/ void CollectionManager::createViewGroup(SysMsgT* msg) {
  using IndexOld = typename SysMsgT::IndexOld;
  using IndexNew = typename SysMsgT::IndexNew;
  using CollectType = typename SysMsgT::CollectionType;

  auto const nb_nodes = theContext()->getNumNodes();
  auto const my_node  = theContext()->getNode();

  // retrieve view creation data
  auto const& old_proxy = msg->old_proxy_;
  auto const& new_proxy = msg->new_proxy_;
  auto const& old_range = msg->old_range_;
  auto const& new_range = msg->new_range_;
  auto const& old_view  = msg->old_view_han_;
  auto const& new_view  = msg->new_view_han_;
  auto const& mapping   = msg->col_map_id_;
  auto const& epoch     = msg->epoch_;
  auto const& tag       = msg->tag_;

  auto const dim = new_range.ndims();
  vtAssert(dim == old_range.ndims(), "Dimension mismatch");

  // save proxy meta-data
  theCollection()->setParent(new_proxy, old_proxy);
  theCollection()->setDim(new_proxy, dim);
  theCollection()->setRange(new_proxy, new_range);

  bool const is_view_old = VirtualProxyBuilder::isView(old_proxy);
  bool const is_view_new = VirtualProxyBuilder::isView(new_proxy);
  vtAssert(not is_view_old, "Nested views not yet supported");
  vtAssert(    is_view_new, "New proxy should be a view one");

  debug_print(
    vrt_coll, node,
    "creating group for view: old_proxy={:x}, new_proxy={:x}\n",
    old_proxy, new_proxy
  );

  //auto const col_node_map = getDefaultMap<CollecType>();
  auto const node_mapping = auto_registry::getHandlerMap(mapping);
  auto const new_filter   = auto_registry::getHandlerView(new_view);
  auto const old_filter   = (is_view_old ? auto_registry::getHandlerView(old_view) : nullptr);

  bool in_group = false;
  auto copy_range = new_range;

  new_range.foreach([&](IndexNew idx) mutable {
    // no need to recheck if already resolved
    if (not in_group) {
      // todo update below if old_proxy is already a view
      auto cur = static_cast<vt::index::BaseIndex*>(&idx);
      auto max = static_cast<vt::index::BaseIndex*>(&copy_range);
      // use the collection mapping to know if current node
      // should be in the new group or not.
      auto const mapped_node = node_mapping(cur, max, nb_nodes);

      if (my_node == mapped_node) {
        //auto old_index = (is_view_old ? old_index_map(cur) : *cur);
        // todo: update here for nested slices
        in_group = new_filter(cur);

        debug_print(
          vrt_coll, node,
          "filtering indices for view: node:{}, idx={}, in_group={}\n",
          my_node, print_index(idx), in_group
        );
      }
    }
  });

  // create the view group
  auto const virtual_id = VirtualProxyBuilder::getVirtualID(new_proxy);

  auto const group_id = theGroup()->newGroupCollective(
    in_group, [new_proxy, virtual_id](GroupType new_group){

      auto const& root       = theGroup()->groupRoot(new_group);
      auto const& is_default = theGroup()->groupDefault(new_group);
      auto const& in_group   = theGroup()->inGroup(new_group);

      // notify all for finalization step
      if (not is_default and in_group) {
        uint64_t const group_tag_mask = 0x0ff0f000;
        TagType const tag = virtual_id | group_tag_mask;
        auto group_msg = makeSharedMessage<ViewGroupMsg>(new_proxy, new_group);

        theGroup()->groupReduce(new_group)->reduce<
          ViewGroupMsg, reduceViewHan
        >(root, group_msg, tag);

      } else if (is_default) {
        // trigger the group finished handler directly.
        auto new_msg = makeMessage<ViewGroupMsg>(new_proxy, new_group);
        theCollection()->finishViewHan<>(new_msg.get());
      }
    }
  );

  debug_print(
    vrt_coll, node,
    "group created for view proxy={:x}, group_id={}\n",
    new_proxy, group_id
  );
  // assign the group id to the new proxy
  theCollection()->assignGroup(new_proxy, group_id);

  // buffer all view pending requests
  theCollection()->bufferViewAction<CollectType>(new_proxy, epoch, tag);
}

template <typename>
/*static*/ void CollectionManager::reduceViewHan(ViewGroupMsg* msg) {

  debug_print(
    vrt_coll, node,
    "reduceViewHan: proxy={:x}, root={}, group={}\n",
    msg->getProxy(), msg->isRoot(), msg->getGroup()
  );

  if (msg->isRoot()) {
    auto new_msg = makeSharedMessage<ViewGroupMsg>(*msg);
    theMsg()->broadcastMsg<ViewGroupMsg,finishViewHan>(new_msg);
  }
  // not sure here
  finishViewHan<>(msg);
}

template <typename>
/*static*/ void CollectionManager::finishViewHan(ViewGroupMsg* msg) {

  auto const& proxy = msg->getProxy();
  bool const is_view = VirtualProxyBuilder::isView(proxy);
  vtAssertExpr(is_view);

  // need an explicitly reference
  auto& buffer_group = theCollection()->buffered_group_;

  // trigger all buffered actions
  auto iter = buffer_group.find(proxy);

  if (iter != buffer_group.end()) {
    for (auto&& action : iter->second) {
      action(proxy);
      debug_print(
        vrt_coll, node,
        "running buffered view group action, proxy: {}\n",
        proxy
      );
    }
    iter->second.clear();
    buffer_group.erase(iter);
  }

  debug_print(
    vrt_coll, node,
    "view group action finished, proxy: {}, epoch:{:x}\n",
    proxy, theMsg()->getEpoch()
  );

  theCollection()->setViewReady(proxy);
}

template <typename ColT>
void CollectionManager::bufferViewAction(
  VirtualProxyType const& proxy,
  EpochType const& epoch,
  TagType const& tag
) {
  // check if view group creation is finished
  bool const is_ready = theCollection()->isViewReady(proxy);

  // Buffer operations if not yet ready
  if (not is_ready) {
    auto iter = buffered_group_.find(proxy);
    if (iter == buffered_group_.end()) {
      buffered_group_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(proxy),
        std::forward_as_tuple(ActionContainerType{})
      );
      iter = buffered_group_.find(proxy);
    }
    vtAssert(iter != buffered_group_.end(), "Must exist");

    debug_print(
      vrt_coll, node,
      "buffering view requests of proxy:{:x}",
      proxy
    );

    auto const cur_epoch = theMsg()->getEpoch();

    iter->second.push_back([=](VirtualProxyType /*ignored*/) {
      theCollection()->bufferViewAction<ColT>(proxy, epoch, tag);
    });
  }
}

inline void CollectionManager::setViewReady(VirtualProxyType const& proxy) {
  view_ready_[proxy] = true;
}

inline bool CollectionManager::isViewReady(VirtualProxyType const& proxy) const {
  auto const& found = view_ready_.find(proxy);
  return (found != view_ready_.end() and found->second);
}

inline void CollectionManager::assignGroup(
  VirtualProxyType const& proxy, GroupType const& group
) {
  view_group_[proxy] = group;
}

inline void CollectionManager::saveViewHandler(
  VirtualProxyType const& proxy, HandlerType const& han
) {
  view_handler_[proxy] = han;
}

inline HandlerType const& CollectionManager::getViewHandler(
  VirtualProxyType const& proxy
) const {
  auto const& found = view_handler_.find(proxy);
  return (found != view_handler_.end() ? found->second : uninitialized_handler);
}

inline VirtualProxyType const& CollectionManager::getParent(
  VirtualProxyType const& proxy
) const {
  auto const& found = view_parent_.find(proxy);
  return (found != view_parent_.end() ? found->second : proxy);
}

inline void CollectionManager::setParent(
  VirtualProxyType const& proxy, VirtualProxyType const& parent
) {
  view_parent_[proxy] = parent;
}

template <typename IndexT>
inline bool operator<(IndexT const& index, IndexT const& range) {
  auto const dim = range.ndims();
  vtAssert(dim > 0, "Invalid index type");
  vtAssert(index.ndims() == dim, "Invalid index type");

  for (int i = 0; i < dim; ++i) {
    if (index[i] >= range[i]) {
      return false;
    }
  }
  return true;
}

template <typename IndexT>
inline bool matches(IndexT const& index, IndexT const& other) {
  auto const dim = other.ndims();
  vtAssert(dim > 0, "Invalid index type");
  vtAssert(index.ndims() == dim, "Invalid index type");

  for (int i = 0; i < dim; ++i) {
    if (not(index[i] == other[i])) {
      return false;
    }
  }
  return true;
}

template <typename IndexT>
inline IndexT const& CollectionManager::getRange(
  VirtualProxyType const& proxy
) const {
  auto const& found = view_range_<IndexT>.find(proxy);
  vtAssert(found != view_range_<IndexT>.end(), "Must be already set");
  return found->second;
}

template <typename IndexT>
void CollectionManager::setRange(
  VirtualProxyType const& proxy, IndexT const& range
) {
  view_range_<IndexT>[proxy] = range;
}

inline int8_t CollectionManager::getDim(VirtualProxyType const& proxy) const {

  auto const& found = view_dimen_.find(proxy);
  vtAssert(found != view_dimen_.end(), "Must be already set");
  return found->second;
}

inline void CollectionManager::setDim(
  VirtualProxyType const& proxy, int8_t dim
) {
  vtAssert(dim > 0, "Invalid dimension");
  view_dimen_[proxy] = dim;
}

template <typename ColT, typename IndexT>
int CollectionManager::getSize(VirtualProxyType const& proxy) const {
  // (!) the collection is assumed to be already built
  bool const is_view = VirtualProxyBuilder::isView(proxy);

  // filter elems if view
  if (is_view) {
    while(not isViewReady(proxy)) {
      runScheduler();
    }

    auto const view_han = getViewHandler(proxy);
    auto const in_slice = auto_registry::getHandlerView(view_han);
    auto const range = getRange<IndexT>(proxy);

    int count = 0;
    range.foreach([&](IndexT idx) {
      auto raw_idx = static_cast<vt::index::BaseIndex*>(&idx);
      if (in_slice(raw_idx)) {
        count++;
      }
    });
    return count;
  } else {
    auto const parent = getParent(proxy);
    vtAssert(parent == proxy, "The proxy should be its own parent");
    auto container = findElmHolder<ColT,IndexT>(parent);
    return container->numElements();
  }
}

template <typename ColT, typename IndexT>
IndexT CollectionManager::resolveIndex(
  VirtualProxyType const& view_proxy,
  VirtualProxyType const& base_proxy,
  IndexT const& new_idx
) const {

  bool const indirect = VirtualProxyBuilder::isView(view_proxy);
  auto const view_range = getRange<IndexT>(view_proxy);
  vtAssert(new_idx < view_range, "Index out-of-range");

  if (indirect) {
    bool const nested = VirtualProxyBuilder::isView(base_proxy);
    vtAssert(not nested, "Both proxies should not be all views");

    // retrieve view data and filtering method
    auto const dimension  = getDim(base_proxy);
    auto const view_han   = getViewHandler(view_proxy);
    auto const in_slice   = auto_registry::getHandlerView(view_han);
    auto const base_range = getRange<IndexT>(base_proxy);

    bool resolved = false;
    IndexT old_idx = {};    // previous value of 'cur_idx'
    IndexT rel_idx = {};    // relative index of 'cur_idx'
    IndexT abs_idx = {};    // absolute index of 'new_idx'

    base_range.foreach([&](IndexT cur_idx) {
      if (not resolved) {
        auto raw_idx = static_cast<vt::index::BaseIndex*>(&cur_idx);

        if (in_slice(raw_idx)) {
          // increment the right component of the relative index
          for (int i = 0; i < dimension; ++i) {
            if (old_idx[i] != cur_idx[i]) {
              rel_idx[i]++;
            }
          }
          // check if relative index matches view index
          if (matches(new_idx, rel_idx)) {
            abs_idx = cur_idx;
            resolved = true;
          }
          old_idx = cur_idx;
        }
      }
    });

    vtAssert(resolved, "Absolute index should be resolved now");
    return abs_idx;
  } else {
    // no indirection in this case
    return new_idx;
  }
}

inline void CollectionManager::insertCollectionInfo(
  VirtualProxyType const& proxy, HandlerType const& map_han,
  EpochType const& insert_epoch
) {
  UniversalIndexHolder<>::insertMap(proxy,map_han,insert_epoch);
}

inline VirtualProxyType CollectionManager::makeNewCollectionProxy(bool is_view) {
  auto const& node = theContext()->getNode();
  return VirtualProxyBuilder::createProxy(
    curIdent_<>++, node, true, true, false, is_view
  );
}

/*
 * Support of virtual context collection element dynamic insertion
 */

template <typename ColT, typename IndexT>
/*static*/ void CollectionManager::insertHandler(InsertMsg<ColT,IndexT>* msg) {
  auto const& epoch = msg->epoch_;
  auto const& g_epoch = msg->g_epoch_;
  theCollection()->insert<ColT,IndexT>(
    msg->proxy_,msg->idx_,msg->construct_node_
  );
  theTerm()->consume(epoch);
  theTerm()->consume(g_epoch);
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

  debug_print(
    vrt_coll, node,
    "finishedInsertEpoch: creating new group: elms={}, in_group={}\n",
    elms, in_group
  );

  theCollection()->createGroupCollection<ColT, IndexT>(untyped_proxy, in_group);

  /*
   *  Contribute to reduction for update epoch
   */
  auto const& root = 0;
  auto nmsg = makeSharedMessage<FinishedUpdateMsg>(untyped_proxy);
  theCollective()->reduce<FinishedUpdateMsg,finishedUpdateHan>(
    root, nmsg, msg->epoch_
  );
}

template <typename>
/*static*/ void CollectionManager::finishedUpdateHan(
  FinishedUpdateMsg* msg
) {
  debug_print(
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
  debug_print(
    vrt_coll, node,
    "setupNextInsertTrigger: proxy={:x}, insert_epoch={}\n",
    proxy, insert_epoch
  );

  auto finished_insert_trigger = [proxy,insert_epoch]{
    debug_print(
      vrt_coll, node,
      "insert finished insert trigger: epoch={}\n",
      insert_epoch
    );
    theCollection()->finishedInsertEpoch<ColT,IndexT>(proxy,insert_epoch);
  };
  auto start_detect = [insert_epoch,finished_insert_trigger]{
    debug_print(
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
  auto const& this_node = theContext()->getNode();
  auto const& untyped_proxy = proxy.getProxy();

  debug_print(
    vrt_coll, node,
    "finishedInsertEpoch: (before) proxy={:x}, epoch={}\n",
    untyped_proxy, epoch
  );

  /*
   *  Add trigger for the next insertion phase/epoch finishing
   */
  auto const& next_insert_epoch = theTerm()->newEpochRooted(false,false);
  UniversalIndexHolder<>::insertSetEpoch(untyped_proxy,next_insert_epoch);

  auto msg = makeSharedMessage<UpdateInsertMsg<ColT,IndexT>>(
    proxy,next_insert_epoch
  );
  theMsg()->broadcastMsg<
    UpdateInsertMsg<ColT,IndexT>,updateInsertEpochHandler
  >(msg);

  /*
   *  Start building the a new group for broadcasts and reductions over the
   *  current set of elements based the distributed snapshot
   */
  auto const elms = theCollection()->groupElementCount<ColT,IndexT>(
    untyped_proxy
  );
  bool const in_group = elms > 0;

  debug_print(
    vrt_coll, node,
    "finishedInsertEpoch: creating new group: elms={}, in_group={}\n",
    elms, in_group
  );

  theCollection()->createGroupCollection<ColT, IndexT>(untyped_proxy, in_group);

  debug_print(
    vrt_coll, node,
    "finishedInsertEpoch: (after broadcast) proxy={:x}, epoch={}\n",
    untyped_proxy, epoch
  );

  /*
   *  Setup next epoch
   */
  setupNextInsertTrigger<ColT,IndexT>(untyped_proxy,next_insert_epoch);

  debug_print(
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
  auto nmsg = makeSharedMessage<FinishedUpdateMsg>(untyped_proxy);
  theCollective()->reduce<FinishedUpdateMsg,finishedUpdateHan>(
    root, nmsg, next_insert_epoch
  );

  debug_print(
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
  debug_print(
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

  debug_print(
    vrt_coll, node,
    "doneInsertHandler: proxy={:x}, node={}\n",
    untyped_proxy, node
  );

  if (node != uninitialized_destination) {
    auto send = [untyped_proxy,node]{
      auto msg = makeSharedMessage<ActInsertMsg<ColT,IndexT>>(untyped_proxy);
      theMsg()->sendMsg<
        ActInsertMsg<ColT,IndexT>,actInsertHandler<ColT,IndexT>
      >(node,msg);
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

  debug_print(
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
    auto msg = makeSharedMessage<DoneInsertMsg<ColT,IndexT>>(proxy,node);
    theMsg()->sendMsg<DoneInsertMsg<ColT,IndexT>,doneInsertHandler<ColT,IndexT>>(
      cons_node,msg
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
  using BaseIdxType      = vt::index::BaseIndex;
  using IdxContextHolder = InsertContextHolder<IndexT>;

  auto const untyped_proxy = proxy.getProxy();
  auto found_constructed = constructed_.find(untyped_proxy) != constructed_.end();

  debug_print(
    vrt_coll, node,
    "insert: proxy={:x}, constructed={}\n",
    untyped_proxy, found_constructed
  );

  if (found_constructed) {
    auto col_holder = findColHolder<ColT,IndexT>(untyped_proxy);
    auto max_idx = col_holder->max_idx;
    auto const& this_node = theContext()->getNode();
    auto map_han = UniversalIndexHolder<>::getMap(untyped_proxy);
    auto insert_epoch = UniversalIndexHolder<>::insertGetEpoch(untyped_proxy);
    vtAssert(insert_epoch != no_epoch, "Epoch should be valid");

    // Get the handler function
    auto fn = auto_registry::getHandlerMap(map_han);

    auto const cur = static_cast<BaseIdxType*>(&idx);
    auto const max = static_cast<BaseIdxType*>(&max_idx);
    auto const& mapped_node = fn(cur, max, theContext()->getNumNodes());

    auto const& has_explicit_node = node != uninitialized_destination;
    auto const& insert_node = has_explicit_node ? node : mapped_node;

    if (insert_node == this_node) {
      auto const& num_elms = max_idx.getSize();
      std::tuple<> tup;

      // Set the current context index to `idx`, enabled the user to query the
      // index during the constructor
      IdxContextHolder::set(&idx,untyped_proxy);

      #if backend_check_enabled(detector)
        auto new_vc = DerefCons::derefTuple<ColT,IndexT,std::tuple<>>(
          num_elms, idx, &tup
        );
      #else
        auto new_vc = CollectionManager::runConstructor<ColT, IndexT>(
          num_elms, idx, &tup, std::make_index_sequence<0>{}
        );
      #endif

      /*
       * Set direct attributes of the newly constructed element directly on
       * the user's class
       */
      CollectionTypeAttorney::setup(new_vc, num_elms, idx, untyped_proxy);

      theCollection()->insertCollectionElement<ColT, IndexT>(
        std::move(new_vc), idx, max_idx, map_han, untyped_proxy, false,
        mapped_node
      );

      // Clear the current index context
      IdxContextHolder::clear();
    } else {
      auto const& cur_epoch = theMsg()->getEpoch();
      auto msg = makeSharedMessage<InsertMsg<ColT,IndexT>>(
        proxy,max_idx,idx,insert_node,mapped_node,insert_epoch,cur_epoch
      );
      theTerm()->produce(insert_epoch);
      theTerm()->produce(cur_epoch);
      theMsg()->sendMsg<InsertMsg<ColT,IndexT>,insertHandler<ColT,IndexT>>(
        insert_node,msg
      );
    }
  } else {
    auto insert_epoch = UniversalIndexHolder<>::insertGetEpoch(untyped_proxy);
    auto iter = buffered_bcasts_.find(untyped_proxy);
    if (iter == buffered_bcasts_.end()) {
      buffered_bcasts_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(untyped_proxy),
        std::forward_as_tuple(ActionContainerType{})
      );
      iter = buffered_bcasts_.find(untyped_proxy);
    }
    vtAssert(iter != buffered_bcasts_.end(), "Must exist");

    debug_print(
      vrt_coll, node,
      "pushing dynamic insertion into buffered sends: {}\n",
      untyped_proxy
    );

    auto const& cur_epoch = theMsg()->getEpoch();
    theTerm()->produce(cur_epoch);
    theTerm()->produce(insert_epoch);

    debug_print(
      vrt_coll, node,
      "insert: proxy={:x}, buffering\n", untyped_proxy
    );
    iter->second.push_back([=](VirtualProxyType /*ignored*/){
      debug_print(
        vrt_coll, node,
        "insert: proxy={:x}, running buffered\n", untyped_proxy
      );
      theCollection()->insert<ColT>(proxy,idx,node);
      theTerm()->consume(insert_epoch);
      theTerm()->consume(cur_epoch);
    });
  }
}

/*
 * Support of virtual context collection element migration
 */

template <typename ColT>
MigrateStatus CollectionManager::migrate(
  VrtElmProxy<ColT, typename ColT::IndexType> proxy, NodeType const& dest
) {
  using IndexT = typename ColT::IndexType;
  auto const& col_proxy = proxy.getCollectionProxy();
  auto const& elm_proxy = proxy.getElementProxy();
  auto const& idx = elm_proxy.getIndex();
  return migrateOut<ColT,IndexT>(col_proxy, idx, dest);
}

template <typename ColT, typename IndexT>
MigrateStatus CollectionManager::migrateOut(
  VirtualProxyType const& col_proxy, IndexT const& idx, NodeType const& dest
) {
 auto const& this_node = theContext()->getNode();

 debug_print(
   vrt_coll, node,
   "migrateOut: col_proxy={:x}, this_node={}, dest={}, idx={}\n",
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

   #if backend_check_enabled(runtime_checks)
   {
     bool const& exists = elm_holder->exists(idx);
     vtAssert(
       exists, "Local element must exist here for migration to occur"
     );
   }
   #endif

   bool const& exists = elm_holder->exists(idx);
   if (!exists) {
     return MigrateStatus::ElementNotLocal;
   }

   debug_print(
     vrt_coll, node,
     "migrateOut: (before remove) holder numElements={}\n",
     elm_holder->numElements()
   );

   auto& coll_elm_info = elm_holder->lookup(idx);
   auto map_fn = coll_elm_info.map_fn;
   auto range = coll_elm_info.max_idx;
   auto col_unique_ptr = elm_holder->remove(idx);
   auto& typed_col_ref = *static_cast<ColT*>(col_unique_ptr.get());

   debug_print(
     vrt_coll, node,
     "migrateOut: (after remove) holder numElements={}\n",
     elm_holder->numElements()
   );

   /*
    * Invoke the virtual prelude migrate out function
    */
   col_unique_ptr->preMigrateOut();

   debug_print(
     vrt_coll, node,
     "migrateOut: col_proxy={:x}, idx={}, dest={}: serializing collection elm\n",
     col_proxy, print_index(idx), dest
   );

   using MigrateMsgType = MigrateMsg<ColT, IndexT>;

   auto msg = makeSharedMessage<MigrateMsgType>(
     proxy, this_node, dest, map_fn, range, &typed_col_ref
   );

   theMsg()->sendMsgAuto<
     MigrateMsgType, MigrateHandlers::migrateInHandler<ColT, IndexT>
   >(dest, msg);

   theLocMan()->getCollectionLM<ColT, IndexT>(col_proxy)->entityMigrated(
     proxy, dest
   );

   /*
    * Invoke the virtual epilog migrate out function
    */
   col_unique_ptr->epiMigrateOut();

   debug_print(
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

   return MigrateStatus::MigratedToRemote;
 } else {
   #if backend_check_enabled(runtime_checks)
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
  debug_print(
    vrt_coll, node,
    "CollectionManager::migrateIn: proxy={:x}, idx={}, from={}, ptr={}\n",
    proxy, print_index(idx), from, print_ptr(vrt_elm_ptr.get())
  );

  auto vc_raw_ptr = vrt_elm_ptr.get();

  /*
   * Invoke the virtual prelude migrate-in function
   */
  vc_raw_ptr->preMigrateIn();

  auto const& elm_proxy = CollectionProxy<ColT, IndexT>(proxy).operator()(
    idx
  );

  bool const is_static = ColT::isStaticSized();
  auto const& home_node = uninitialized_destination;
  auto const& inserted = insertCollectionElement<ColT, IndexT>(
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
  theMsg()->broadcastMsg<DestroyMsgType, DestroyHandlers::destroyNow>(msg.get());
  DestroyHandlers::destroyNow(msg.get());
}

template <typename ColT, typename IndexT>
void CollectionManager::incomingDestroy(
  CollectionProxyWrapType<ColT,IndexT> const& proxy
) {
  destroyMatching<ColT,IndexT>(proxy);
}

template <typename ColT, typename IndexT>
void CollectionManager::destroyMatching(
  CollectionProxyWrapType<ColT,IndexT> const& proxy
) {
  UniversalIndexHolder<>::destroyCollection(proxy.getProxy());
  auto elm_holder = findElmHolder<ColT,IndexT>(proxy.getProxy());
  elm_holder->destroyAll();
}

template <typename ColT, typename IndexT>
CollectionHolder<ColT, IndexT>* CollectionManager::findColHolder(
  VirtualProxyType const& proxy
) const {
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
) const {
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
) const {
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
void CollectionManager::nextPhase(
  CollectionProxyWrapType<ColT, typename ColT::IndexType> const& proxy,
  PhaseType const& cur_phase, ActionFinishedLBType continuation
) {
  using namespace balance;
  using MsgType = PhaseMsg<ColT>;
  auto msg = makeSharedMessage<MsgType>(cur_phase, proxy);
  auto const& instrument = false;

  debug_print(
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

  backend_enable_if(
    lblite, {
      msg->setLBLiteInstrument(instrument);
      debug_print(
        vrt_coll, node,
        "nextPhase: broadcasting: instrument={}, cur_phase={}\n",
        msg->lbLiteInstrument(), cur_phase
      );
    }
  );
  theCollection()->broadcastMsg<MsgType,ElementStats::syncNextPhase<ColT>>(
    proxy, msg, instrument
  );
}

template <typename ColT>
void CollectionManager::computeStats(
  CollectionProxyWrapType<ColT, typename ColT::IndexType> const& proxy,
  PhaseType const& cur_phase
) {
  using namespace balance;
  using MsgType = PhaseMsg<ColT>;
  auto msg = makeSharedMessage<MsgType>(cur_phase,proxy);
  auto const& instrument = false;

  debug_print(
    vrt_coll, node,
    "computeStats: broadcasting: cur_phase={}\n",
    cur_phase
  );

  theCollection()->broadcastMsg<MsgType,ElementStats::computeStats<ColT>>(
    proxy, msg, nullptr, instrument
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
  UniversalIndexHolder<>::resetPhase();
  if (lb_continuations_.size() > 0) {
    auto continuations = lb_continuations_;
    lb_continuations_.clear();
    for (auto&& elm : continuations) {
      theTerm()->consume(term::any_epoch_sentinel);
      elm();
    }
  }
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

template <typename always_void>
void CollectionManager::schedule(ActionType action) {
  work_units_.push_back(action);
}

template <typename always_void>
bool CollectionManager::scheduler() {
  if (work_units_.size() == 0) {
    return false;
  } else {
    auto unit = work_units_.back();
    work_units_.pop_back();
    unit();
    return true;
  }
}

inline DispatchBasePtrType
getDispatcher(auto_registry::AutoHandlerType const& han) {
  return theCollection()->getDispatcher(han);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_IMPL_H*/
