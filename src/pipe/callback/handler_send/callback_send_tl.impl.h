
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_TL_IMPL_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_TL_IMPL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/callback_base_tl.h"
#include "pipe/callback/handler_send/callback_send_tl.h"
#include "activefn/activefn.h"
#include "context/context.h"
#include "messaging/active.h"
#include "runnable/general.h"

namespace vt { namespace pipe { namespace callback {

template <typename SerializerT>
void CallbackSendTypeless::serialize(SerializerT& s) {
  s | send_node_;
  s | handler_;
}

template <typename MsgT>
void CallbackSendTypeless::trigger(MsgT* msg, PipeType const& pipe) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    pipe, node,
    "CallbackSendTypeless: trigger_: pipe={:x}, this_node={}, send_node_={}\n",
    pipe, this_node, send_node_
  );
  if (this_node == send_node_) {
    auto nmsg = reinterpret_cast<ShortMessage*>(msg);
    runnable::Runnable<ShortMessage>::run(handler_, nullptr, nmsg, this_node);
  } else {
    theMsg()->sendMsgAuto<MsgT>(send_node_, handler_, msg);
  }
}

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_TL_IMPL_H*/
