
#if !defined INCLUDED_CONTEXT_VRT_MANAGER
#define INCLUDED_CONTEXT_VRT_MANAGER

#include <unordered_map>
#include <memory>

#include "config.h"
#include "utils/bits/bits_common.h"
#include "context.h"
#include "context_vrt.h"
#include "registry_function.h"
#include "context_vrtmessage.h"
#include "auto_registry_vc.h"

#include "location/location.h"

#include "context_vrtproxy.h"

namespace vt { namespace vrt {

struct VrtContextManager {
  using VrtContextPtrType = std::unique_ptr<VrtContext>;
  using VrtContextManager_ContainerType = std::unordered_map<
    VrtContext_IdType, VrtContextPtrType
  >;

  VrtContextManager();

  static void handleVCMsg(BaseMessage* msg);

  template <typename VrtContextT, typename... Args>
  VrtContext_ProxyType constructVrtContext(Args&& ... args);

  VrtContext* getVrtContextByID(VrtContext_IdType const& lookupID);
  VrtContext* getVrtContextByProxy(VrtContext_ProxyType const& proxy);
  void destroyVrtContextByID(VrtContext_IdType const& lookupID);
  void destroyVrtContextByProxy(VrtContext_ProxyType const& proxy);

  NodeType getNode() const;
  VrtContext_IdType getCurrentIdent() const;

  template <typename VcT, typename MsgT, ActiveVCFunctionType<MsgT, VcT> *f>
  void sendMsg(
    VrtContext_ProxyType const& toProxy, MsgT *const msg,
    ActionType act = nullptr
  );

 private:
  VrtContextManager_ContainerType holder_;
  VrtContext_IdType curIdent_;
  NodeType myNode_;
};

}}  // end namespace vt::vrt

namespace vt {

extern std::unique_ptr<vrt::VrtContextManager> theVrtCManager;

}  // end namespace vt

#include "context_vrtmanager.impl.h"

#endif  /*INCLUDED_CONTEXT_VRT_MANAGER*/
