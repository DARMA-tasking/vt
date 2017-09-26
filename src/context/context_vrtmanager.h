
#if !defined INCLUDED_CONTEXT_VRT_MANAGER
#define INCLUDED_CONTEXT_VRT_MANAGER

#include <unordered_map>

#include "config.h"
#include "utils/bits/bits_common.h"
#include "context.h"
#include "context_vrt.h"

#include "location/location.h"

#include "context_vrtproxy.h"


namespace vt { namespace vrt {

struct VrtContextManager {
  using VrtContextManager_ContainerType
  = std::unordered_map<VrtContext_IdType, VrtContext*>;

  VrtContextManager();

  template <typename VrtContextT, typename... Args>
  VrtContext_ProxyType constructVrtContext(Args&& ... args) {
    holder_[curIdent_] = new VrtContextT{args...};

//    theLocMan->vrtContextLoc->registerEntity();
//    theLocMan->virtual_loc->registerEntity()

    curIdent_++;
    return VrtContextProxy::createNewProxy(curIdent_ - 1, myNode_);
  }

  VrtContext* getVrtContextByID(VrtContext_IdType const& lookupID);
  VrtContext* getVrtContextByProxy(VrtContext_ProxyType const& proxy);
  void destroyVrtContextByID(VrtContext_IdType const& lookupID);
  void destroyVrtContextByProxy(VrtContext_ProxyType const& proxy);

  NodeType getNode() const;
  VrtContext_IdType getCurrentIdent() const;

//  template <typename VrtCntxT, typename MsgT, ActiveAnyFunctionType<MsgT>* f>
//  EventType sendMsg(MsgT* const msg,
//                    TagType const& tag = no_tag,
//                    ActionType next_action = nullptr) {
//    HandlerType const& han = auto_registry::makeAutoHandler<MsgT,f>(msg);
//    auto const& this_node = theContext->getNode();
//    setBroadcastType(msg->env);
//    if (tag != no_tag) {
//      envelopeSetTag(msg->env, tag);
//    }
//    return sendMsg(this_node, han, msg, next_action);
//  }


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