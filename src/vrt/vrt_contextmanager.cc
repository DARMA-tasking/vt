
#include "vrt_contextmanager.h"

namespace vt {
namespace vrt {

VrtContextManager::VrtContextManager() {
  curIdent_ = 0;
  myNode_ = theContext->getNode();
}

VrtContext*VrtContextManager::getVrtContextByID
    (VrtContext_IdType const& lookupID) {
  VrtContextManager_ContainerType::const_iterator got = holder_.find(lookupID);
  if (got == holder_.end()) {
    return nullptr;
  } else {
    return got->second;
  }
}

void VrtContextManager::destroyVrtContextByID
    (VrtContext_IdType const& lookupID) {
  holder_.erase(holder_.find(lookupID));
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