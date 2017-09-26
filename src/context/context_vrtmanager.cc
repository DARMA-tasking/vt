
#include "context_vrtmanager.h"

namespace vt {
namespace vrt {

VrtContextManager::VrtContextManager() {
  curIdent_ = 0;
  myNode_ = theContext->getNode();
}

VrtContext*VrtContextManager::getVrtContextByID(
    VrtContext_IdType const& lookupID) {
  VrtContextManager_ContainerType::const_iterator got = holder_.find(lookupID);
  VrtContext* answer = nullptr;
  if (got != holder_.end()) {
    answer = got->second;
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

std::unique_ptr<vrt::VrtContextManager> theVrtCManager;

} // end namespace vt::vrt