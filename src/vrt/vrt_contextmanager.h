

#if !defined INCLUDED_VRT_CONTEXT_MANAGER
#define INCLUDED_VRT_CONTEXT_MANAGER

#include <vector>

#include "vrt_context.h"

#include "config.h"
#include "utils/bits/bits_common.h"
#include "configs/types/types_headers.h"
#include "transport.h"

namespace vt { namespace vrt {

struct VrtContextManager {
  using VrtContext_Type = VrtContext;
  using VrtContext_UniversalIdType = VrtContextType;
  using VrtContextManager_ContainerType = std::vector<VrtContext_Type>;

  VrtContextManager() {
    curIdent_ = 0;
    myNode_ = theContext->getNode();
  }

  inline VrtContext_UniversalIdType newVrtContext() {
    holder_.emplace_back(VrtContext(myNode_, curIdent_));
    return curIdent_++;
  }

  inline void newRemoteVrtContext(
      NodeType const& node, bool const& is_coll = false,
      bool const& is_migratable = false) {}

 private:
  VrtContextManager_ContainerType holder_;
  VrtContext_UniversalIdType curIdent_;
  NodeType myNode_;
};

}}  // end namespace vt::vrt

namespace vt {

extern std::unique_ptr<vrt::VrtContextManager> theVrtCManager;

}  // end namespace vt

#endif  /*INCLUDED_VRT_CONTEXT_MANAGER*/