
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_IMPL_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_IMPL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/handler_send/callback_send.h"
#include "context/context.h"
#include "messaging/active.h"

namespace vt { namespace pipe { namespace callback {

template <typename MsgT>
CallbackSend<MsgT>::CallbackSend(
  HandlerType const& in_handler, NodeType const& in_send_node
) : send_node_(in_send_node), handler_(in_handler)
{ }

template <typename MsgT>
template <typename SerializerT>
void CallbackSend<MsgT>::serialize(SerializerT& s) {
  CallbackBase<SignalBaseType>::serializer(s);
  s | send_node_;
  s | handler_;
}

template <typename MsgT>
void CallbackSend<MsgT>::trigger_(SignalDataType* data) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    pipe, node,
    "CallbackSend: trigger_: this_node={}, send_node_={}\n",
    this_node, send_node_
  );
  if (this_node == send_node_) {
    auto msg = reinterpret_cast<ShortMessage*>(data);
    runnable::Runnable<ShortMessage>::run(handler_, nullptr, msg, this_node);
  } else {
    theMsg()->sendMsg<SignalDataType>(send_node_, handler_, data);
  }
}

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_IMPL_H*/
