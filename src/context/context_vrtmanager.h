
#if !defined INCLUDED_CONTEXT_VRT_MANAGER
#define INCLUDED_CONTEXT_VRT_MANAGER

#include <unordered_map>

#include "config.h"
#include "utils/bits/bits_common.h"
#include "context.h"
#include "context_vrt.h"

namespace vt { namespace vrt {

using VrtContext_IdType = uint32_t;

struct VrtContextManager {
  using VrtContextManager_ContainerType
  = std::unordered_map<VrtContext_IdType, VrtContext*>;

  VrtContextManager();

  template <typename VrtContextT, typename... Args>
  VrtContext_IdType constructVrtContext(Args&& ... args) {
    holder_[curIdent_] = new VrtContextT{args...};
    holder_[curIdent_]->setVrtContextNode(myNode_);
    return curIdent_++;
  };

  VrtContext* getVrtContextByID(VrtContext_IdType const& lookupID);
  void destroyVrtContextByID(VrtContext_IdType const& lookupID);

  NodeType getNode() const;
  VrtContext_IdType getCurrentIdent() const;


 private:
  VrtContextManager_ContainerType holder_;
  VrtContext_IdType curIdent_;
  NodeType myNode_;
};

}}  // end namespace vt::vrt

namespace vt {

extern std::unique_ptr<vrt::VrtContextManager> theVrtCManager;

}  // end namespace vt

#endif  /*INCLUDED_CONTEXT_VRT_MANAGER*/