
#include "context_vrtmanager.h"

namespace vt {
namespace vrt {

VirtualContextManager::VirtualContextManager()
  : curIdent_(0), myNode_(theContext->getNode())
{ }

/*static*/ void VirtualContextManager::virtualMsgHandler(BaseMessage* msg) {
  auto const vc_msg = static_cast<VirtualMessage*>(msg);
  auto const entity_proxy = vc_msg->getEntity();
  auto const vc_ptr = theVirtualManager->getVirtualByProxy(entity_proxy);

  debug_print(
    vrt, node,
    "handleVCMsg: msg=%p, entity_proxy=%lld, vc_ptr=%p\n",
    msg, entity_proxy, vc_ptr
  );

  if (vc_ptr) {
    // invoke the user's handler function here
    auto const& sub_handler = vc_msg->getHandler();
    auto const& vc_active_fn = auto_registry::getAutoHandlerVC(sub_handler);
    // execute the user's handler with the message and VC ptr
    vc_active_fn(static_cast<VirtualMessage*>(msg), vc_ptr);
  } else {
    // the VC does not exist here?
    assert(false && "A virtual context must exist to invoke user handler");
  }
}

VirtualContext*VirtualContextManager::getVirtualByID(
    VirtualIDType const& lookupID) {
  ContainerType::const_iterator got = holder_.find(lookupID);
  VirtualContext* answer = nullptr;
  if (got != holder_.end()) {
    answer = got->second.get();
  } else {
    // upps not found
  }
  return answer;
}

VirtualContext* VirtualContextManager::getVirtualByProxy(
    VirtualProxyType const& proxy) {
  VirtualContext* answer = nullptr;
  if (VirtualProxyBuilder::getVirtualNode(proxy) == myNode_) {
    answer = getVirtualByID(VirtualProxyBuilder::getVirtualID(proxy));
  } else {
    // this proxy is not on this node
  }
  return answer;
}

void VirtualContextManager::destroyVirtualByID(
    VirtualIDType const& lookupID) {
  holder_.erase(holder_.find(lookupID));
}

void VirtualContextManager::destoryVirtualByProxy(
    VirtualProxyType const& proxy) {
  if (VirtualProxyBuilder::getVirtualNode(proxy) == myNode_) {
    destroyVirtualByID(VirtualProxyBuilder::getVirtualID(proxy));
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

}

} // end namespace vt::vrt
