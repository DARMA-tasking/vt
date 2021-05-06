/*
//@HEADER
// *****************************************************************************
//
//                                  manager.cc
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

#include "vt/config.h"
#include "vt/configs/arguments/app_config.h"
#include "vt/runtime/runtime.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/base/base.h"
#include "vt/vrt/collection/manager.h"
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"

namespace vt { namespace vrt { namespace collection {

CollectionManager::CollectionManager() { }

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
    normal, vrt_coll,
    "reduceConstruction: invoke reduce: proxy={:x}\n", proxy
  );

  using collective::reduce::makeStamp;
  using collective::reduce::StrongUserID;

  auto stamp = makeStamp<StrongUserID>(proxy);
  auto r = theCollection()->reducer();
  auto cb = theCB()->makeBcast<CollectionConsMsg, collectionFinishedHan>();
  r->reduce<collective::None>(root, msg.get(), cb, stamp);
}

void CollectionManager::incomingDestroy(VirtualProxyType proxy) {
  auto iter = cleanup_fns_.find(proxy);
  if (iter != cleanup_fns_.end()) {
    auto fns = std::move(iter->second);
    cleanup_fns_.erase(iter);
    for (auto fn : fns) {
      fn();
    }
  }
}

void CollectionManager::destroy(VirtualProxyType proxy) {
  auto const& this_node = theContext()->getNode();

  auto msg = makeMessage<DestroyMsg>(proxy, this_node);
  theMsg()->markAsCollectionMessage(msg);
  theMsg()->broadcastMsg<DestroyMsg, DestroyHandlers::destroyNow>(msg);
}

/*static*/ void CollectionManager::actInsertHandler(ActInsertMsg* msg) {
  auto const& proxy = msg->proxy_;
  return theCollection()->actInsert(proxy);
}

void CollectionManager::actInsert(VirtualProxyType const& proxy) {
  vt_debug_print(
    verbose, vrt_coll,
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

/*static*/ void CollectionManager::finishedUpdateHan(FinishedUpdateMsg* msg) {
  vt_debug_print(
    verbose, vrt_coll,
    "finishedUpdateHan: proxy={:x}, root={}\n",
    msg->proxy_, msg->isRoot()
  );

  if (msg->isRoot()) {
    /*
     *  Trigger any actions that the user may have registered for when insertion
     *  has fully terminated
     */
    return theCollection()->actInsert(msg->proxy_);
  }
}

/*static*/ void CollectionManager::doneInsertHandler(DoneInsertMsg* msg) {
  auto const& node = msg->action_node_;
  auto const& untyped_proxy = msg->proxy_;

  vt_debug_print(
    verbose, vrt_coll,
    "doneInsertHandler: proxy={:x}, node={}\n",
    untyped_proxy, node
  );

  if (node != uninitialized_destination) {
    auto send = [untyped_proxy,node]{
      auto smsg = makeMessage<ActInsertMsg>(untyped_proxy);
      theMsg()->markAsCollectionMessage(smsg);
      theMsg()->sendMsg<ActInsertMsg, actInsertHandler>(node, smsg);
    };
    return theCollection()->finishedInserting(msg->proxy_, send);
  } else {
    return theCollection()->finishedInserting(msg->proxy_);
  }
}

void CollectionManager::finishedInserting(
  VirtualProxyType proxy, ActionType insert_action
) {
  auto const& this_node = theContext()->getNode();
  /*
   *  Register the user's action for when insertion is completed across the
   *  whole system, which termination in the insertion epoch can enforce
   */
  if (insert_action) {
    auto iter = user_insert_action_.find(proxy);
    if (iter ==  user_insert_action_.end()) {
      user_insert_action_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(proxy),
        std::forward_as_tuple(ActionVecType{insert_action})
      );
    } else {
      iter->second.push_back(insert_action);
    }
  }

  auto const& cons_node = VirtualProxyBuilder::getVirtualNode(proxy);

  vt_debug_print(
    verbose, vrt_coll,
    "finishedInserting: proxy={:x}, cons_node={}, this_node={}\n",
    proxy, cons_node, this_node
  );

  if (cons_node == this_node) {
    auto iter = insert_finished_action_.find(proxy);
    if (iter != insert_finished_action_.end()) {
      auto action_lst = iter->second;
      insert_finished_action_.erase(proxy);
      for (auto&& action : action_lst) {
        action();
      }
    }
  } else {
    auto node = insert_action ? this_node : uninitialized_destination;
    auto msg = makeMessage<DoneInsertMsg>(proxy,node);
    theMsg()->markAsCollectionMessage(msg);
    theMsg()->sendMsg<DoneInsertMsg,doneInsertHandler>(cons_node, msg);
  }
}

bool CollectionManager::checkReady(
  VirtualProxyType proxy, BufferReleaseEnum release
) {
  return getReadyBits(proxy, release) == release;
}

BufferReleaseEnum CollectionManager::getReadyBits(
  VirtualProxyType proxy, BufferReleaseEnum release
) {
  auto release_state = getState(proxy);
  auto ret = static_cast<BufferReleaseEnum>(release & release_state);

  vt_debug_print(
    verbose, vrt_coll,
    "getReadyBits: proxy={:x}, check release={:b}, state={:b}, ret={:b}\n",
    proxy, release, release_state, ret
  );

  return ret;
}

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
    return bufferOp(proxy, type, release, epoch, action);
  }
}

void CollectionManager::triggerReadyOps(
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

/*static*/ void CollectionManager::collectionGroupFinishedHan(
  CollectionGroupMsg* msg
) {
  auto const& proxy = msg->getProxy();
  theCollection()->addToState(proxy, BufferReleaseEnum::AfterGroupReady);
  theCollection()->triggerReadyOps(proxy, BufferTypeEnum::Reduce);
}

/*static*/ void CollectionManager::collectionFinishedHan(
  CollectionConsMsg* msg
) {
  auto const& proxy = msg->proxy;
  theCollection()->constructed_.insert(proxy);
  theCollection()->addToState(proxy, BufferReleaseEnum::AfterFullyConstructed);

  vt_debug_print(
    verbose, vrt_coll,
    "addToState: proxy={:x}, AfterCons\n", proxy
  );

  theCollection()->triggerReadyOps(proxy, BufferTypeEnum::Broadcast);
  theCollection()->triggerReadyOps(proxy, BufferTypeEnum::Send);
  theCollection()->triggerReadyOps(proxy, BufferTypeEnum::Reduce);
}

/*static*/ void CollectionManager::collectionGroupReduceHan(
  CollectionGroupMsg* msg
) {
  vt_debug_print(
    verbose, vrt_coll,
    "collectionGroupReduceHan: proxy={:x}, root={}, group={}\n",
    msg->proxy, msg->isRoot(), msg->getGroup()
  );
  if (msg->isRoot()) {
    auto nmsg = makeMessage<CollectionGroupMsg>(*msg);
    theMsg()->markAsCollectionMessage(nmsg);
    theMsg()->broadcastMsg<CollectionGroupMsg,collectionGroupFinishedHan>(
      nmsg
    );
  }
}

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

void CollectionManager::destroyCollections() {
  UniversalIndexHolder<>::destroyAllLive();
}

void CollectionManager::finalize() {
  cleanupAll();
}

/*virtual*/ CollectionManager::~CollectionManager() { }

void CollectionManager::startup() {
#if vt_check_enabled(lblite)
  // First hook, do all stat manipulation
  thePhase()->registerHookCollective(phase::PhaseHook::End, []{
    auto const& map = theCollection()->collect_stats_for_lb_;
    for (auto&& elm : map) {
      // this will trigger all the data collection required for LB
      elm.second();
    }
  });

  // Second hook, select and then potentially start the LB
  thePhase()->registerHookCollective(phase::PhaseHook::End, []{
    auto const cur_phase = thePhase()->getCurrentPhase();
    theLBManager()->selectStartLB(cur_phase);
  });
#endif
}

DispatchBasePtrType
getDispatcher(auto_registry::AutoHandlerType const han) {
  return theCollection()->getDispatcher(han);
}

balance::ElementIDStruct CollectionManager::getCurrentContext() const {
# if vt_check_enabled(lblite)
  if (theContext()->getTask() != nullptr) {
    auto lb = theContext()->getTask()->get<ctx::LBStats>();
    if (lb != nullptr) {
      return lb->getCurrentElementID();
    }
  }
#endif
  return balance::ElementIDStruct{
    balance::no_element_id, uninitialized_destination, uninitialized_destination
  };
}

void CollectionManager::schedule(ActionType action) {
  theSched()->enqueue(action);
}

}}} /* end namespace vt::vrt::collection */
