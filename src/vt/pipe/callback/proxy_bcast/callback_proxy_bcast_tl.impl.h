
#if !defined INCLUDED_PIPE_CALLBACK_PROXY_BCAST_CALLBACK_PROXY_BCAST_TL_IMPL_H
#define INCLUDED_PIPE_CALLBACK_PROXY_BCAST_CALLBACK_PROXY_BCAST_TL_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/callback_base_tl.h"
#include "vt/pipe/callback/proxy_bcast/callback_proxy_bcast_tl.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/vrt/collection/manager.h"
#include "vt/vrt/collection/dispatch/dispatch.h"
#include "vt/vrt/collection/dispatch/registry.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"

#include <cassert>

namespace vt { namespace pipe { namespace callback {

template <typename SerializerT>
void CallbackProxyBcastTypeless::serialize(SerializerT& s) {}

template <typename MsgT>
void CallbackProxyBcastTypeless::trigger(MsgT* msg, PipeType const& pipe) {
  auto const& this_node = theContext()->getNode();
  auto const& pipe_node = PipeIDBuilder::getNode(pipe);
  debug_print(
    pipe, node,
    "CallbackProxyBcastTypeless: trigger_: pipe={:x}, this_node={}\n",
    pipe, this_node
  );
  if (this_node == pipe_node) {
    theCB()->triggerPipeTyped<MsgT>(pipe,msg);
  } else {
    /*
     * Set pipe type on the message envelope; use the group in the envelope in
     * indicate the pipe
     */
    setPipeType(msg->env);
    envelopeSetGroup(msg->env,pipe);
    theMsg()->sendMsgAuto<MsgT,PipeManager::triggerCallbackMsgHan>(
      pipe_node,msg
    );
  }
}

template <typename SerializerT>
void CallbackProxyBcastDirect::serialize(SerializerT& s) {
  s | handler_;
  s | vrt_dispatch_han_;
  s | member_;
  s | proxy_;
}

template <typename MsgT>
void CallbackProxyBcastDirect::trigger(MsgT* msg, PipeType const& pipe) {
  auto const& this_node = theContext()->getNode();
  auto const& pipe_node = PipeIDBuilder::getNode(pipe);
  debug_print(
    pipe, node,
    "CallbackProxyBcastDirect: trigger_: pipe={:x}, this_node={}, "
    "handler={}, vrt_handler={}\n",
    pipe, this_node, handler_, vrt_dispatch_han_
  );
  if (this_node == pipe_node) {
    theCB()->triggerPipeTyped<MsgT>(pipe,msg);
  } else {
    auto dispatcher = theCollection()->getDispatcher(vrt_dispatch_han_);
    auto const& proxy = proxy_;
    dispatcher->broadcast(proxy,msg,handler_,member_,nullptr);
  }
}

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_PROXY_BCAST_CALLBACK_PROXY_BCAST_TL_IMPL_H*/
