
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_TL_IMPL_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_TL_IMPL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/callback_base_tl.h"
#include "pipe/callback/handler_bcast/callback_bcast_tl.h"
#include "activefn/activefn.h"
#include "context/context.h"
#include "messaging/active.h"
#include "runnable/general.h"

namespace vt { namespace pipe { namespace callback {

template <typename SerializerT>
void CallbackBcastTypeless::serialize(SerializerT& s) {
  s | include_sender_;
  s | handler_;
}

template <typename MsgT>
void CallbackBcastTypeless::trigger(MsgT* msg, PipeType const& pipe) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    pipe, node,
    "CallbackBcast: trigger_: pipe={:x}, this_node={}, include_sender_={}\n",
    pipe, this_node, include_sender_
  );
  theMsg()->broadcastMsg<MsgT>(handler_, msg);
  if (include_sender_) {
    auto nmsg = makeSharedMessage<MsgT>(*msg);
    auto nmsgc = reinterpret_cast<ShortMessage*>(nmsg);
    messageRef(nmsg);
    runnable::Runnable<ShortMessage>::run(handler_,nullptr,nmsgc,this_node);
    messageDeref(nmsg);
  }
}

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_TL_IMPL_H*/
