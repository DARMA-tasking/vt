
#include "config.h"
#include "vrt/context/context_vrtmanager.h"
#include "vrt/context/context_vrt_attorney.h"
#include "topos/location/location_headers.h"

namespace vt { namespace vrt {

VirtualContextManager::VirtualContextManager()
  : curIdent_(0), myNode_(theContext()->getNode())
{ }

VirtualProxyType VirtualContextManager::makeVirtualPlaceholder() {
  auto const& proxy = generateNewProxy();
  insertVirtualContext(nullptr, proxy);
  return proxy;
}

void VirtualContextManager::insertVirtualContext(
  typename VirtualContextManager::VirtualPtrType new_vc, VirtualProxyType proxy
) {
  auto const& is_remote = VirtualProxyBuilder::isRemote(proxy);
  auto const& id = VirtualProxyBuilder::getVirtualID(proxy);

  auto holder_iter = holder_.find(id);
  assert(holder_iter == holder_.end() && "Holder must not contain id");

  // registry the proxy with location manager
  location::LocationManager::vrtContextLoc->registerEntity(
    proxy, virtualMsgHandler
  );

  bool const is_constructed = new_vc != nullptr;

  if (is_constructed) {
    // save the proxy in the virtual context for reference later
    VirtualContextAttorney::setProxy(new_vc.get(), proxy);
  }

  debug_print(
    vrt, node,
    "inserting new VC into holder_: id=%d, proxy=%lld, construct=%s, ptr=%p\n",
    id, proxy, print_bool(is_constructed),
    is_constructed ? new_vc.get() : nullptr
  );

  auto& holder = is_remote ? remote_holder_ : holder_;

  auto info = std::make_unique<VirtualInfoType>(
    is_constructed ? std::move(new_vc) : nullptr, proxy, !is_constructed
  );

  // insert into the holder at the current slot
  holder.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(std::move(info))
  );
}

VirtualProxyType VirtualContextManager::generateNewProxy() {
  return VirtualProxyBuilder::createProxy(curIdent_++, myNode_);
}

VirtualRemoteIDType VirtualContextManager::generateNewRemoteID(
  NodeType const& node
) {
  auto iter = curRemoteID_.find(node);
  if (iter == curRemoteID_.end()) {
    // Must start at 1 to avoid conflicts
    curRemoteID_.emplace(node, 1);
    iter = curRemoteID_.find(node);
  }
  return iter->second++;
}

/*static*/ void VirtualContextManager::virtualMsgHandler(BaseMessage* msg) {
  auto const vc_msg = static_cast<VirtualMessage*>(msg);
  auto const entity_proxy = vc_msg->getProxy();

  auto vc_info = theVirtualManager()->getVirtualInfoByProxy(entity_proxy);

  assert(
    vc_info != nullptr && "A virtual context must exist to invoke user handler"
  );

  debug_print(
    vrt, node,
    "virtualMsgHandler: msg=%p, entity_proxy=%lld\n",
    msg, entity_proxy
  );

  vc_info->tryEnqueueWorkUnit(static_cast<VirtualMessage*>(msg));
}

/*static*/ void VirtualContextManager::sendBackVirtualProxyHan(
  VirtualProxyRequestMsg* msg
) {
  auto const& cons_node = msg->construct_node;
  auto const& req_node = msg->request_node;
  auto const& request_id = msg->request_id;

  debug_print(
    vrt, node,
    "sendBackVirtualProxy: send back cons_node=%d, req_node=%d, id=%lld\n",
    cons_node, req_node, request_id
  );

  theVirtualManager()->recvVirtualProxy(request_id, msg->proxy);
}

void VirtualContextManager::recvVirtualProxy(
  VirtualRequestIDType id, VirtualProxyType proxy
) {
  auto pending_iter = pending_request_.find(id);

  assert(
    pending_iter == pending_request_.end() && "Must be valid pending request"
  );

  auto& pending_req = pending_iter->second;
  pending_req.action(proxy);

  pending_request_.erase(pending_iter);
}

VirtualContextManager::VirtualInfoType*
VirtualContextManager::getVirtualInfoByID(
  VirtualIDType const& lookupID, bool const is_remote
) {
  auto& holder = is_remote ? remote_holder_ : holder_;
  auto iter = holder.find(lookupID);
  if (iter == holder.end()) {
    assert(0 && "Virtual entity could not be found locally: invalid ID!");
    return nullptr;
  } else {
    return iter->second.get();
  }
}

VirtualContext* VirtualContextManager::getVirtualByID(
  VirtualIDType const& lookupID, bool const is_remote
) {
  return getVirtualInfoByID(lookupID, is_remote)->get();
}

VirtualContextManager::VirtualInfoType*
VirtualContextManager::getVirtualInfoByProxy(VirtualProxyType const& proxy) {
  auto const& is_remote = VirtualProxyBuilder::isRemote(proxy);
  if (VirtualProxyBuilder::getVirtualNode(proxy) == myNode_) {
    auto const& id = VirtualProxyBuilder::getVirtualID(proxy);
    auto info = getVirtualInfoByID(id, is_remote);
    return info;
  } else {
    // this proxy is not on this node
    assert(0 && "Proxy must be on this node");
    return nullptr;
  }
}

VirtualContext* VirtualContextManager::getVirtualByProxy(
    VirtualProxyType const& proxy
) {
  return getVirtualInfoByProxy(proxy)->get();
}

void VirtualContextManager::destroyVirtualByID(
  VirtualIDType const& lookupID, bool const is_remote
) {
  auto& holder = is_remote ? remote_holder_ : holder_;
  auto iter = holder.find(lookupID);
  assert(iter != holder.end() and "Virtual ID must exist");
  holder.erase(iter);
}

void VirtualContextManager::destoryVirtualByProxy(
    VirtualProxyType const& proxy
) {
  auto const& is_remote = VirtualProxyBuilder::isRemote(proxy);
  if (VirtualProxyBuilder::getVirtualNode(proxy) == myNode_) {
    destroyVirtualByID(VirtualProxyBuilder::getVirtualID(proxy), is_remote);
  } else {
    // this proxy is not on this node
  }
}

NodeType VirtualContextManager::getNode() const {
  return myNode_;
}

VirtualIDType VirtualContextManager::getCurrentID() const {
  return curIdent_;
}

}} // end namespace vt::vrt
