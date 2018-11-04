
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_TL_IMPL_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_TL_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/callback_base_tl.h"
#include "vt/pipe/callback/handler_bcast/callback_bcast_tl.h"
#include "vt/activefn/activefn.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/runnable/general.h"

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
  theMsg()->broadcastMsgAuto<MsgT>(handler_, msg);
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
