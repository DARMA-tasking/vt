/*
//@HEADER
// *****************************************************************************
//
//                            context_vrtmanager.cc
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
#include "vt/vrt/context/context_vrtmanager.h"
#include "vt/vrt/context/context_vrt_attorney.h"
#include "vt/topos/location/location_headers.h"

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
  vtAssert(holder_iter == holder_.end(), "Holder must not contain id");

  // registry the proxy with location manager
  theLocMan()->vrtContextLoc->registerEntity(
    proxy, myNode_, virtualMsgHandler
  );

  bool const is_constructed = new_vc != nullptr;

  if (is_constructed) {
    // save the proxy in the virtual context for reference later
    VirtualContextAttorney::setProxy(new_vc.get(), proxy);
  }

  debug_print(
    vrt, node,
    "inserting new VC into holder_: id={}, proxy={}, construct={}, ptr={}\n",
    id, proxy, print_bool(is_constructed),
    is_constructed ? print_ptr(new_vc.get()) : nullptr
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

  vtAssert(
    vc_info != nullptr, "A virtual context must exist to invoke user handler"
  );

  debug_print(
    vrt, node,
    "virtualMsgHandler: msg={}, entity_proxy={}\n",
    print_ptr(msg), entity_proxy
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
    "sendBackVirtualProxy: send back cons_node={}, req_node={}, id={}\n",
    cons_node, req_node, request_id
  );

  theVirtualManager()->recvVirtualProxy(request_id, msg->proxy);
}

void VirtualContextManager::recvVirtualProxy(
  VirtualRequestIDType id, VirtualProxyType proxy
) {
  auto pending_iter = pending_request_.find(id);

  vtAssert(
    pending_iter == pending_request_.end(), "Must be valid pending request"
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
    vtAssert(0, "Virtual entity could not be found locally: invalid ID!");
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
    vtAssert(0, "Proxy must be on this node");
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
  vtAssert(iter != holder.end(), "Virtual ID must exist");
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
