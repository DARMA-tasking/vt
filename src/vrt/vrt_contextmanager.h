

#if !defined INCLUDED_VRT_CONTEXT_MANAGER
#define INCLUDED_VRT_CONTEXT_MANAGER

#include <vector>

#include "vrt_context.h"

#include "config.h"
#include "utils/bits/bits_common.h"
#include "configs/types/types_headers.h"
#include "context.h"

namespace vt { namespace vrt {

struct VrtContextManager {
  using VrtContextManager_ContainerType = std::vector<VrtContext>;

  VrtContextManager();

  VrtContext newVrtContext();

  template <typename VrtCntxt>
  VrtContext newVrtContext(VrtCntxt* vrtc) {
    VrtContext& temp_vrtc = *static_cast<VrtContext*>(vrtc);
    temp_vrtc.setVrtContextNode(myNode_);
    temp_vrtc.setVrtContextIdentifier(curIdent_);
    holder_.push_back(temp_vrtc);
    return holder_[curIdent_++];
  }

  NodeType                  getNode() const;
  VrtContext_IdentifierType getCurrentIdent() const;


  inline void newRemoteVrtContext(
      NodeType const& node, bool const& is_coll = false,
      bool const& is_migratable = false) {}

  /*|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/

//  inline


 private:
  VrtContextManager_ContainerType holder_;
  VrtContext_IdentifierType curIdent_;
  NodeType myNode_;
};

}}  // end namespace vt::vrt

namespace vt {

extern std::unique_ptr<vrt::VrtContextManager> theVrtCManager;

}  // end namespace vt

#endif  /*INCLUDED_VRT_CONTEXT_MANAGER*/