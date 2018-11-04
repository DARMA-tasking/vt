
#if !defined INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_IMPL_H
#define INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/callback_base_tl.h"
#include "vt/pipe/callback/proxy_send/callback_proxy_send_tl.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"

#include <cassert>

namespace vt { namespace pipe { namespace callback {

template <typename SerializerT>
void CallbackProxySendTypeless::serialize(SerializerT& s) { }

template <typename MsgT>
void CallbackProxySendTypeless::trigger(MsgT* msg, PipeType const& pipe) {
  auto const& this_node = theContext()->getNode();
  auto const& pipe_node = PipeIDBuilder::getNode(pipe);
  debug_print(
    pipe, node,
    "CallbackProxySendTypeless: trigger_: pipe={:x}, this_node={}\n",
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
    theMsg()->sendMsg<MsgT,PipeManager::triggerCallbackMsgHan>(pipe_node,msg);
  }
}

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_IMPL_H*/
