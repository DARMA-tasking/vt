
#if !defined INCLUDED_PIPE_CALLBACK_PROXY_BCAST_CALLBACK_PROXY_BCAST_TL_IMPL_H
#define INCLUDED_PIPE_CALLBACK_PROXY_BCAST_CALLBACK_PROXY_BCAST_TL_IMPL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/callback_base_tl.h"
#include "pipe/callback/proxy_bcast/callback_proxy_bcast_tl.h"
#include "pipe/id/pipe_id.h"
#include "pipe/pipe_manager.h"
#include "context/context.h"
#include "messaging/active.h"

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
    theMsg()->sendMsg<MsgT,PipeManager::triggerCallbackMsgHan>(pipe_node,msg);
  }
}

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_PROXY_BCAST_CALLBACK_PROXY_BCAST_TL_IMPL_H*/
