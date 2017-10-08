
#if !defined INCLUDED_CONTEXT_VRT_MANAGER
#define INCLUDED_CONTEXT_VRT_MANAGER

#include <unordered_map>
#include <memory>

#include "config.h"
#include "context.h"
#include "context_vrt.h"
#include "context_vrt_funcs.h"
#include "context_vrtmessage.h"
#include "context_vrtinfo.h"
#include "context_vrtproxy.h"

#include "utils/bits/bits_common.h"
#include "activefn/activefn.h"
#include "mapping_function.h"

namespace vt { namespace vrt {

struct VrtContextManager {
  using VrtContextPtrType = std::unique_ptr<VrtContext>;
  using VrtInfoType = VrtInfo;
  using VrtContextManager_ContainerType = std::unordered_map<
    VrtContext_IdType, VrtInfoType
  >;

  VrtContextManager();

  static void handleVCMsg(BaseMessage* msg);

  template <typename VrtContextT, typename... Args>
  VrtContext_ProxyType constructVrtContext(Args&& ... args);

  template <typename VrtContextT, mapping::ActiveSeedMapFnType fn, typename... Args>
  VrtContext_ProxyType constructVrtContextWorkerMap(Args&& ... args);

  VrtContext* getVrtContextByProxy(VrtContext_ProxyType const& proxy);
  void destroyVrtContextByProxy(VrtContext_ProxyType const& proxy);

  template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
  void sendMsg(
    VrtContext_ProxyType const& toProxy, MsgT *const msg,
    ActionType act = nullptr
  );

private:
  NodeType getNode() const;
  VrtContext* getVrtContextByID(VrtContext_IdType const& lookupID);
  void destroyVrtContextByID(VrtContext_IdType const& lookupID);
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

#include "context_vrtmanager.impl.h"

#endif  /*INCLUDED_CONTEXT_VRT_MANAGER*/
