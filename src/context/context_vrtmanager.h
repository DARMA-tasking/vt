
#if !defined INCLUDED_CONTEXT_VRT_MANAGER
#define INCLUDED_CONTEXT_VRT_MANAGER

#include <unordered_map>

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

struct VrtContextManager;

}}

namespace vt {

extern std::unique_ptr<vrt::VrtContextManager> theVrtCManager;

}  // end namespace vt

namespace vt { namespace vrt {

struct VrtContextManager {
  using VrtContextManager_ContainerType
  = std::unordered_map<VrtContext_IdType, VrtContext*>;

  VrtContextManager();

  static void handleVCMsg(BaseMessage* msg);

  template <typename VrtContextT, typename... Args>
  VrtContext_ProxyType constructVrtContext(Args&& ... args) {
    holder_[curIdent_] = new VrtContextT{args...};
    holder_[curIdent_]->myProxy_ =
        VrtContextProxy::createNewProxy(curIdent_, myNode_);

    theLocMan->vrtContextLoc->registerEntity(
      holder_[curIdent_]->myProxy_, handleVCMsg
    );

    return holder_[curIdent_++]->myProxy_;
  }

  VrtContext* getVrtContextByID(VrtContext_IdType const& lookupID);
  VrtContext* getVrtContextByProxy(VrtContext_ProxyType const& proxy);
  void destroyVrtContextByID(VrtContext_IdType const& lookupID);
  void destroyVrtContextByProxy(VrtContext_ProxyType const& proxy);

  NodeType getNode() const;
  VrtContext_IdType getCurrentIdent() const;

  template <
    typename VirtualT,
    typename MsgT,
    ActiveVCFunctionType<MsgT, VirtualT> *f
  >
  void sendMsg(
    VrtContext_ProxyType const& toProxy, MsgT *const msg,
    ActionType act = nullptr
  ) {
    NodeType const& home_node = VrtContextProxy::getVrtContextNode(toProxy);
    // register the user's handler
    HandlerType const& han = auto_registry::makeAutoHandlerVC<VirtualT,MsgT,f>(msg);
    // save the user's handler in the message
    msg->setHandler(han);
    theLocMan->vrtContextLoc->routeMsg(toProxy, home_node, msg, act);
  }

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

#endif  /*INCLUDED_CONTEXT_VRT_MANAGER*/
