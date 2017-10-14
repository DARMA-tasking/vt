
#if !defined INCLUDED_CONTEXT_VRT_MANAGER_IMPL
#define INCLUDED_CONTEXT_VRT_MANAGER_IMPL

#include "config.h"
#include "context_vrtmanager.h"
#include "context_vrt_internal_msgs.h"
#include "context_vrt_remoteinfo.h"
#include "topos/location/location.h"
#include "registry/auto_registry_vc.h"
#include "registry/auto_registry_map.h"
#include "serialization/serialization.h"

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

template <typename SystemTuple, typename VrtContextT>
/*static*/ void VirtualContextManager::remoteConstructVrt(
  VirtualConstructDataMsg<SystemTuple>* msg, VrtContextT* ctx
) {
  printf("remoteConstructVrt: unwind tuple and construct\n");

  using Args = typename std::tuple_element<0, SystemTuple>::type;
  static constexpr auto size = std::tuple_size<Args>::value;
  auto new_vc = VirtualContextManager::runConstructor<VrtContextT>(
    &std::get<0>(*msg->tup), std::make_index_sequence<size>{}
  );

  auto const& info = std::get<1>(*msg->tup);
  VirtualProxyType new_proxy = info.proxy;

  if (info.isImmediate) {
    // nothing to do here?
  } else {
    auto const& cons_node = theContext->getNode();
    auto const& req_node = info.from_node;
    auto const& request_id = info.req_id;

    new_proxy = theVirtualManager->generateNewProxy();
    auto send_msg = makeSharedMessage<VirtualProxyRequestMsg>(
      cons_node, req_node, request_id, new_proxy
    );
    theMsg->sendMsg<VirtualProxyRequestMsg, sendBackVirtualProxyHan>(
      req_node, send_msg
    );
  }

  theVirtualManager->insertVirtualContext(std::move(new_vc), new_proxy);
}

template <typename VrtContextT, typename... Args>
VirtualProxyType VirtualContextManager::makeVirtualNode(
  NodeType const& node, Args&& ... args
) {
  auto const& this_node = theContext->getNode();
  if (node != this_node) {
    return makeVirtualRemote<VrtContextT>(
      node, true, nullptr, std::forward<Args>(args)...
    );
  } else {
    return makeVirtual<VrtContextT>(std::forward<Args>(args)...);
  }
}

template <typename VrtContextT, typename... Args>
VirtualProxyType VirtualContextManager::makeVirtualRemote(
  NodeType const& dest, bool isImmediate, ActionProxyType action, Args&&... args
) {
  using ArgsTupleType = std::tuple<typename std::decay<Args>::type...>;
  using SystemTupleType = std::tuple<ArgsTupleType, RemoteVrtInfo>;
  using TupleType = SystemTupleType;

  auto const& this_node = theContext->getNode();
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

  SerializedMessenger::sendSerialVirtualMsg<
    VrtContextT, VirtualConstructDataMsg<TupleType>,
    remoteConstructVrt<TupleType, VrtContextT>
  >(dest, TupleType{ArgsTupleType{std::forward<Args>(args)...}, *info.get()});

  return return_proxy;
}

template <typename VrtContextT, mapping::ActiveSeedMapFnType fn, typename... Args>
VirtualProxyType VirtualContextManager::makeVirtualMap(
  Args&& ... args
) {
  auto const next_seed = cur_seed_++;
  //auto mapped_core = fn(next_seed, num_nodes);
  auto const& core_map_handle = auto_registry::makeAutoHandlerSeedMap<fn>();
  auto const& proxy = makeVirtual<VrtContextT, Args...>(
    std::forward<Args>(args)...
  );
  auto const& vrt_id = VirtualProxyBuilder::getVirtualID(proxy);
  auto holder_iter = holder_.find(vrt_id);
  assert(holder_iter != holder_.end() && "Proxy ID Must exist here");
  auto& info = holder_iter->second;
  info.core_map_handler_ = core_map_handle;
  // @todo: actually do the mapping
  auto vrt = getVirtualByProxy(proxy);
  vrt->seed_ = next_seed;
  return proxy;
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
void VirtualContextManager::sendMsg(
  VirtualProxyType const& toProxy, MsgT *const msg, ActionType act
) {
  // @todo: implement the action `act' after the routing is finished

  NodeType const& home_node = VirtualProxyBuilder::getVirtualNode(toProxy);
  // register the user's handler
  HandlerType const& han = auto_registry::makeAutoHandlerVC<VcT,MsgT,f>(msg);
  // save the user's handler in the message
  msg->setHandler(han);

  debug_print(
    vrt, node,
    "sending msg to VC: msg=%p, han=%d, home_node=%d\n", msg, han, home_node
  );

  // route the message to the destination using the location manager
  theLocMan->vrtContextLoc->routeMsg(toProxy, home_node, msg, act);
}

}}  // end namespace vt::vrt

#endif /*INCLUDED_CONTEXT_VRT_MANAGER_IMPL*/
