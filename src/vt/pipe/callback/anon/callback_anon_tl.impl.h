
#if !defined INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_TL_IMPL_H
#define INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_TL_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/callback_base_tl.h"
#include "vt/pipe/callback/anon/callback_anon_tl.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/pipe/msg/callback.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/activefn/activefn.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/runnable/general.h"

namespace vt { namespace pipe { namespace callback {

template <typename SerializerT>
void CallbackAnonTypeless::serialize(SerializerT& s) {}

template <typename MsgT>
void CallbackAnonTypeless::trigger(MsgT* msg, PipeType const& pipe) {
  auto const& this_node = theContext()->getNode();
  auto const& pipe_node = PipeIDBuilder::getNode(pipe);
  debug_print(
    pipe, node,
    "CallbackAnonTypeless: trigger_: pipe={:x}, this_node={}\n",
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

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_TL_IMPL_H*/
