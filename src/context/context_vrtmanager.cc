
#include "context_vrtmanager.h"

namespace vt {
namespace vrt {

VrtContextManager::VrtContextManager() {
  curIdent_ = 0;
  myNode_ = theContext->getNode();
}

/*static*/ void VrtContextManager::handleVCMsg(BaseMessage* msg) {
  auto const vc_msg = static_cast<VrtContextMessage*>(msg);
  auto const entity_proxy = vc_msg->getEntity();
  auto const vc_ptr = theVrtCManager->getVrtContextByProxy(entity_proxy);

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
    vc_active_fn(static_cast<VrtContextMessage*>(msg), vc_ptr);
  } else {
    // the VC does not exist here?
    assert(false && "A virtual context must exist to invoke user handler");
  }
}

VrtContext*VrtContextManager::getVrtContextByID(
    VrtContext_IdType const& lookupID) {
  VrtContextManager_ContainerType::const_iterator got = holder_.find(lookupID);
  VrtContext* answer = nullptr;
  if (got != holder_.end()) {
    answer = got->second.get();
  } else {
    // upps not found
  }
  return answer;
}

VrtContext* VrtContextManager::getVrtContextByProxy(
    VrtContext_ProxyType const& proxy) {
  VrtContext* answer = nullptr;
  if (VrtContextProxy::getVrtContextNode(proxy) == myNode_) {
    answer = getVrtContextByID(VrtContextProxy::getVrtContextId(proxy));
  } else {
    // this proxy is not on this node
  }
  return answer;
}

void VrtContextManager::destroyVrtContextByID(
    VrtContext_IdType const& lookupID) {
  holder_.erase(holder_.find(lookupID));
}

void VrtContextManager::destroyVrtContextByProxy(
    VrtContext_ProxyType const& proxy) {
  if (VrtContextProxy::getVrtContextNode(proxy) == myNode_) {
    destroyVrtContextByID(VrtContextProxy::getVrtContextId(proxy));
  } else {
    // this proxy is not on this node
  }
}

NodeType VrtContextManager::getNode() const {
  return myNode_;
}

VrtContext_IdType VrtContextManager::getCurrentIdent() const {
  return curIdent_;
}

}

} // end namespace vt::vrt
