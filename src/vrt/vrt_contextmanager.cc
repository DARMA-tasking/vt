
#include "vrt_contextmanager.h"

namespace vt { namespace vrt {

VrtContextManager::VrtContextManager() {
  curIdent_ = 0;
  myNode_ = theContext->getNode();
}

VrtContext VrtContextManager::newVrtContext() {
  holder_.emplace_back(VrtContext(myNode_, curIdent_));
  return holder_[curIdent_++];
}

NodeType VrtContextManager::getNode() const {
  return myNode_;
}

VrtContext_IdentifierType VrtContextManager::getCurrentIdent() const {
  return curIdent_;
}


}

std::unique_ptr<vrt::VrtContextManager> theVrtCManager;

} // end namespace vt::vrt