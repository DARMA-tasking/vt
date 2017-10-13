
#if !defined INCLUDED_CONTEXT_VRT_MANAGER_IMPL
#define INCLUDED_CONTEXT_VRT_MANAGER_IMPL

#include "config.h"
#include "context_vrtmanager.h"
#include "topos/location/location.h"
#include "registry/auto_registry_vc.h"
#include "registry/auto_registry_map.h"

#include <cassert>
#include <memory>

namespace vt { namespace vrt {

template <typename VrtContextT, typename... Args>
VirtualProxyType VirtualContextManager::makeVirtual(Args&& ... args) {
  auto holder_iter = holder_.find(curIdent_);
  assert(
    holder_iter == holder_.end() &&
    "Holder must not contain curIdent_: should be impossible"
  );

  auto new_vc = std::make_unique<VrtContextT>(std::forward<Args>(args)...);
  auto const& proxy = VirtualProxyBuilder::createProxy(curIdent_, myNode_);

  // registry the proxy with location manager
  theLocMan->vrtContextLoc->registerEntity(proxy, virtualMsgHandler);

  // save the proxy in the virtual context for reference later
  new_vc->proxy_ = proxy;

  debug_print(
    vrt, node,
    "inserting new VC into holder_: ident=%d, proxy=%lld, ptr=%p\n",
    curIdent_, proxy, new_vc.get()
  );

  // insert into the holder at the current slot, and increment slot
  holder_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(curIdent_),
    std::forward_as_tuple(VirtualInfoType{std::move(new_vc), proxy})
  );

  curIdent_++;

  return proxy;
}

template <typename VrtContextT, typename MessageT>
VirtualProxyType VirtualContextManager::makeVirtualMsg(
  NodeType const& node, MessageT* m
) {
  return VirtualProxyType{};
}

template <typename VrtContextT, mapping::ActiveSeedMapFnType fn, typename... Args>
VirtualProxyType VirtualContextManager::makeVirtualMap(
  Args&& ... args
) {
  auto const& core_map_handle = auto_registry::makeAutoHandlerSeedMap<fn>();
  auto const& proxy = makeVirtual<VrtContextT, Args...>(
    std::forward<Args>(args)...
  );
  auto const& vrt_id = VirtualProxyBuilder::getVirtualID(proxy);
  auto holder_iter = holder_.find(vrt_id);
  assert(holder_iter != holder_.end() && "Proxy ID Must exist here");
  auto& info = holder_iter->second;
  info.core_map_handler_ = core_map_handle;
  // @todo: do the actual mapping
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
