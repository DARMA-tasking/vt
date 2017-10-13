
#if !defined INCLUDED_CONTEXT_VRT_MANAGER
#define INCLUDED_CONTEXT_VRT_MANAGER

#include <unordered_map>
#include <memory>

#include "config.h"
#include "context/context.h"
#include "context_vrt.h"
#include "context_vrt_funcs.h"
#include "context_vrtmessage.h"
#include "context_vrtinfo.h"
#include "context_vrtproxy.h"

#include "utils/bits/bits_common.h"
#include "activefn/activefn.h"
#include "topos/mapping/mapping_function.h"

namespace vt { namespace vrt {

struct VirtualContextManager {
  using VirtualPtrType = std::unique_ptr<VirtualContext>;
  using VirtualInfoType = VirtualInfo;
  using ContainerType = std::unordered_map<VirtualIDType, VirtualInfoType>;

  VirtualContextManager();

  template <typename VrtContextT, typename... Args>
  VirtualProxyType makeVirtual(Args&& ... args);

  template <typename VrtContextT, typename MessageT>
  VirtualProxyType makeVirtualMsg(NodeType const& node, MessageT* m);

  template <typename VrtContextT, mapping::ActiveSeedMapFnType fn, typename... Args>
  VirtualProxyType makeVirtualMap(Args&& ... args);

  VirtualContext* getVirtualByProxy(VirtualProxyType const& proxy);
  void destoryVirtualByProxy(VirtualProxyType const& proxy);

  template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
  void sendMsg(
    VirtualProxyType const& toProxy, MsgT *const msg, ActionType act = nullptr
  );

private:
  // All messages directed to a virtual context are routed through this handler
  // so the user's handler can be invoked with the pointer to the virtual
  // context
  static void virtualMsgHandler(BaseMessage* msg);

  VirtualContext* getVirtualByID(VirtualIDType const& lookupID);
  void destroyVirtualByID(VirtualIDType const& lookupID);
  VirtualIDType getCurrentID() const;
  NodeType getNode() const;

 private:
  // Holder for local virtual contexts that are mapped to this node; VirtualInfo
  // holds a pointer to the virtual along with other meta-information about it
  ContainerType holder_;

  // The current identifier (node-local) for this manager
  VirtualIDType curIdent_;

  // Cache of the node for the virtual context manager
  NodeType myNode_;
};

}}  // end namespace vt::vrt

namespace vt {

extern std::unique_ptr<vrt::VirtualContextManager> theVirtualManager;

}  // end namespace vt

#include "context_vrtmanager.impl.h"

#endif  /*INCLUDED_CONTEXT_VRT_MANAGER*/
