/*
//@HEADER
// *****************************************************************************
//
//                          context_vrtmanager.impl.h
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

#if !defined INCLUDED_CONTEXT_VRT_MANAGER_IMPL
#define INCLUDED_CONTEXT_VRT_MANAGER_IMPL

#include "vt/config.h"
#include "vt/vrt/context/context_vrtmanager.h"
#include "vt/vrt/context/context_vrt_internal_msgs.h"
#include "vt/vrt/context/context_vrt_remoteinfo.h"
#include "vt/vrt/context/context_vrt_make_closure.h"
#include "vt/topos/location/location_headers.h"
#include "vt/topos/location/manager.h"
#include "vt/registry/auto/vc/auto_registry_vc.h"
#include "vt/registry/auto/map/auto_registry_map.h"
#include "vt/worker/worker_headers.h"

#include <cassert>
#include <memory>

namespace vt { namespace vrt {

template <typename VrtContextT, typename... Args>
VirtualProxyType VirtualContextManager::makeVirtual(Args&& ... args) {
  auto new_vc = std::make_unique<VrtContextT>(std::forward<Args>(args)...);
  auto const& proxy = generateNewProxy();
  insertVirtualContext(std::move(new_vc), proxy);
  return proxy;
}

using namespace ::vt::serialization;

template <typename VrtCtxT, typename Tuple, size_t... I>
/*static*/ typename VirtualContextManager::VirtualPtrType
VirtualContextManager::runConstructor(
  Tuple* tup, std::index_sequence<I...>
) {
  return std::make_unique<VrtCtxT>(
    std::forward<typename std::tuple_element<I,Tuple>::type>(
      std::get<I>(*tup)
    )...
  );
}

template <typename SysMsgT>
/*static*/ void VirtualContextManager::remoteConstructVrt(SysMsgT* msg) {
  using VrtContextT = typename SysMsgT::VirtualContextType;
  using Args = typename SysMsgT::ArgsTupleType;

  static constexpr auto size = std::tuple_size<Args>::value;
  auto new_vc = VirtualContextManager::runConstructor<VrtContextT>(
    &msg->tup, std::make_index_sequence<size>{}
  );

  auto const& info = msg->info;
  VirtualProxyType new_proxy = info.proxy;

  if (info.isImmediate) {
    // nothing to do here?
  } else {
    auto const& cons_node = theContext()->getNode();
    auto const& req_node = info.from_node;
    auto const& request_id = info.req_id;

    new_proxy = theVirtualManager()->generateNewProxy();
    auto send_msg = makeMessage<VirtualProxyRequestMsg>(
      cons_node, req_node, request_id, new_proxy
    );
    theMsg()->sendMsg<VirtualProxyRequestMsg, sendBackVirtualProxyHan>(
      req_node, send_msg.get()
    );
  }

  theVirtualManager()->insertVirtualContext(std::move(new_vc), new_proxy);
}

template <typename VrtContextT, typename... Args>
VirtualProxyType VirtualContextManager::makeVirtualNode(
  NodeType const& node, Args&& ... args
) {
  auto const& this_node = theContext()->getNode();
  if (node != this_node) {
    return makeVirtualRemote<VrtContextT>(
      node, true, nullptr, std::forward<Args>(args)...
    );
  } else {
    return makeVirtual<VrtContextT>(std::forward<Args>(args)...);
  }
}

template <typename MsgT>
/*static*/ void VirtualContextManager::virtualTypedMsgHandler(MsgT* msg) {
  VirtualContextManager::virtualMsgHandler(reinterpret_cast<BaseMessage*>(msg));
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
messaging::PendingSend VirtualContextManager::sendSerialMsg(
  VirtualProxyType const& toProxy, MsgT *const msg
) {
  auto base_msg = promoteMsg(msg).template to<BaseMsgType>();
  ByteType msg_sz = sizeof(MsgT);

  if (theContext()->getWorker() == worker_id_comm_thread) {
    NodeType const& home_node = VirtualProxyBuilder::getVirtualNode(toProxy);
    // register the user's handler
    HandlerType const& han = auto_registry::makeAutoHandlerVC<VcT,MsgT,f>(msg);
    // save the user's handler in the message
    msg->setVrtHandler(han);
    msg->setProxy(toProxy);

    debug_print(
      vrt, node,
      "sending serialized msg to VC: msg={}, han={}, home_node={}, toProxy={}\n",
      print_ptr(msg), han, home_node, toProxy
    );

    using SerialMsgT = SerializedEagerMsg<MsgT, VirtualMessage>;

    // route the message to the destination using the location manager
    messaging::PendingSend pending(
      base_msg, msg_sz, [=](MsgPtr<BaseMsgType> mymsg){
        // Uses special implementation overload not exposed in theMsg..
        MsgT* typed_msg = reinterpret_cast<MsgT*>(mymsg.get());
        auto sendSerialHan = auto_registry::makeAutoHandler<MsgT,virtualTypedMsgHandler<MsgT>>(nullptr);
        SerializedMessenger::sendSerialMsgSendImpl<MsgT, VirtualMessage>(
          typed_msg,
          sendSerialHan,
          // custom send lambda to route the message
          [=](MsgSharedPtr<SerialMsgT> innermsg) -> messaging::PendingSend {
            innermsg->setProxy(toProxy);
            theLocMan()->vrtContextLoc->routeMsgHandler<
              SerialMsgT, SerializedMessenger::payloadMsgHandler
            >(toProxy, home_node, innermsg.get());
            return messaging::PendingSend(nullptr);
          },
          // custom data transfer lambda if above the eager threshold
          [=](ActionNodeSendType action) -> messaging::PendingSend {
            auto captured_action = [=](NodeType node){ action(node); };
            theLocMan()->vrtContextLoc->routeNonEagerAction(
              toProxy, home_node, captured_action
            );
            return messaging::PendingSend(nullptr);
          }
        );
      }
    );
    return pending;
  } else {
    return messaging::PendingSend(
      base_msg, msg_sz, [=](MsgPtr<BaseMsgType> mymsg) {
        theWorkerGrp()->enqueueCommThread([=]{
          auto typed_msg = reinterpret_cast<MsgT*>(mymsg.get());
          theVirtualManager()->sendSerialMsg<VcT, MsgT, f>(toProxy, typed_msg);
        });
      }
    );
  }
}

template <typename VrtContextT, typename... Args>
VirtualProxyType VirtualContextManager::makeVirtualRemote(
  NodeType const& dest, bool isImmediate, ActionProxyType action, Args&&... args
) {
  using ArgsTupleType = std::tuple<typename std::decay<Args>::type...>;
  using MsgType = VrtConstructMsg<RemoteVrtInfo, ArgsTupleType, VrtContextT>;

  auto sys_msg = makeMessage<MsgType>(ArgsTupleType{std::forward<Args>(args)...});

  auto const& this_node = theContext()->getNode();
  std::unique_ptr<RemoteVrtInfo> info = nullptr;
  VirtualProxyType return_proxy = no_vrt_proxy;

  if (isImmediate) {
    auto const& remote_id = generateNewRemoteID(dest);
    return_proxy = VirtualProxyBuilder::createRemoteProxy(
      remote_id, myNode_, dest, false, false
    );
    info = std::make_unique<RemoteVrtInfo>(this_node, return_proxy);
  } else {
    auto next_req = cur_request_id++;

    // insert a pending request to trigger the action
    pending_request_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(next_req),
      std::forward_as_tuple(PendingRequestType{next_req, action})
    );

    info = std::make_unique<RemoteVrtInfo>(this_node, next_req);
  }

  sys_msg->info = *info.get();

  theMsg()->sendMsg<MsgType,remoteConstructVrt<MsgType>>(dest, sys_msg.get());

  return return_proxy;
}

inline void VirtualContextManager::setupMappedVirutalContext(
  VirtualProxyType const& proxy, SeedType const& seed, CoreType const& core,
  HandlerType const& map_handle
) {
  auto vrt_info = getVirtualInfoByProxy(proxy);
  vrt_info->setSeed(seed);
  vrt_info->setCoreMap(map_handle);
  vrt_info->mapToCore(core);
}

template <typename VrtContextT, typename... Args>
VirtualProxyType VirtualContextManager::makeVirtualMapComm(
  SeedType const& seed, HandlerType const& map_handle, Args&& ... args
) {
  auto const& proxy = makeVirtual<VrtContextT, Args...>(
    std::forward<Args>(args)...
  );
  setupMappedVirutalContext(proxy, seed, worker_id_comm_thread, map_handle);
  return proxy;
}

template <typename VrtContextT, mapping::ActiveSeedMapFnType fn, typename... Args>
VirtualProxyType VirtualContextManager::makeVirtualMap(Args... args) {
  SeedType next_seed = no_seed;
  HandlerType core_map_handle = uninitialized_handler;
  bool const& has_workers = theContext()->hasWorkers();

  debug_print(
    vrt, node,
    "makeVirtualMap: has_workers={}\n", print_bool(has_workers)
  );

  if (has_workers) {
    next_seed = cur_seed_++;
    core_map_handle = auto_registry::makeAutoHandlerSeedMap<fn>();

    auto const& num_workers = theContext()->getNumWorkers();
    auto const& mapped_core = fn(next_seed, num_workers);

    debug_print(
      vrt, node,
      "seed={}, mapped_core={}, num_workers={}\n",
      next_seed, mapped_core, num_workers
    );

    if (mapped_core != worker_id_comm_thread) {
      using TupleType = std::tuple<Args...>;

      auto proxy = makeVirtualPlaceholder();
      auto vrt_info = getVirtualInfoByProxy(proxy);
      setupMappedVirutalContext(proxy, next_seed, mapped_core, core_map_handle);

      auto cl = new VirtualMakeClosure<VrtContextT, Args...>(
        TupleType{std::forward<Args>(args)...}, proxy, vrt_info
      );

      // work to defer to the worker thread
      auto work_unit = [=]{ cl->make(); delete cl; };
      theWorkerGrp()->enqueueForWorker(mapped_core, work_unit);

      return proxy;
    }
  }

  return makeVirtualMapComm<VrtContextT>(
    next_seed, core_map_handle, std::forward<Args>(args)...
  );
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
messaging::PendingSend VirtualContextManager::sendMsg(
  VirtualProxyType const& toProxy, MsgT *const raw_msg
) {
  auto msg = promoteMsg(raw_msg);

  auto const& home_node = VirtualProxyBuilder::getVirtualNode(toProxy);
  // register the user's handler
  auto const& han = auto_registry::makeAutoHandlerVC<VcT,MsgT,f>(msg.get());
  // save the user's handler in the message
  msg->setVrtHandler(han);
  msg->setProxy(toProxy);

  debug_print(
    vrt, node,
    "sending msg to VC: msg={}, han={}, home_node={}\n",
    print_ptr(msg.get()), han, home_node
  );

  auto base_msg = msg.template to<BaseMsgType>();
  ByteType msg_sz = sizeof(MsgT);
  return messaging::PendingSend(base_msg, msg_sz,
    [=](MsgPtr<BaseMsgType> mymsg){
      // route the message to the destination using the location manager
      auto msg_shared = promoteMsg(reinterpret_cast<MsgT*>(mymsg.get()));
      theLocMan()->vrtContextLoc->routeMsg(toProxy, home_node, msg_shared);
    }
  );
}

}}  // end namespace vt::vrt

#endif /*INCLUDED_CONTEXT_VRT_MANAGER_IMPL*/
