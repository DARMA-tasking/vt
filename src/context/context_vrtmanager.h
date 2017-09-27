
#if !defined INCLUDED_CONTEXT_VRT_MANAGER
#define INCLUDED_CONTEXT_VRT_MANAGER

#include <unordered_map>

#include "config.h"
#include "utils/bits/bits_common.h"
#include "context.h"
#include "context_vrt.h"
#include "function.h"
#include "context_vrtmessage.h"
#include "auto_registry.h"

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

  static void handleVCMsg(BaseMessage* msg) {
    auto const vc_msg = static_cast<VrtContextMessage*>(msg);
    auto const entity_proxy = vc_msg->getEntity();
    auto const vc_ptr = theVrtCManager->getVrtContextByProxy(entity_proxy);

    if (vc_ptr) {
      // invoke the user's handler function here
      auto const& sub_handler = vc_msg->getHandler();
      auto const& fn = auto_registry::getAutoHandler(sub_handler);
      auto vc_active_fn =
        reinterpret_cast<ActiveVCFunctionType<VrtContextMessage, VrtContext>*>(fn);
      // execute the user's handler with the message and VC ptr
      vc_active_fn(static_cast<VrtContextMessage*>(msg), vc_ptr);
    } else {
      // the VC does not exist here?
      assert(0 and "This should never happen");
    }
  }

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

//  theVrtCManager->sendMsg<MyHelloMsg, myWorkHandler>
//  (proxy1, proxy_on_node1, makeSharedMessage<MyHelloMsg>(100));

  template <
    typename VirtualContextT,
    typename MsgT,
    ActiveVCFunctionType<MsgT, VirtualContextT> *f
  >
  EventType sendMsg(
    VrtContext_ProxyType const& toProxy, MsgT *const in_msg,
    ActionType act = nullptr
  ) {
    auto home_node = VrtContextProxy::getVrtContextNode(toProxy);
    //auto f2 = reinterpret_cast<ActiveAnyFunctionType<MsgT>*>(f);
    HandlerType const& han = auto_registry::makeAutoHandler<MsgT,f>(in_msg);
    in_msg->setHandler(han);
    theLocMan->vrtContextLoc->routeMsg(toProxy, home_node, in_msg, act);
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
