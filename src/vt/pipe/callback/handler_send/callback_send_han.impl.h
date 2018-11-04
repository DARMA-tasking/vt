
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_HAN_IMPL_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_HAN_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/callback/handler_send/callback_send_han.h"
#include "vt/pipe/callback/callback_base.h"
#include "vt/activefn/activefn.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"

namespace vt { namespace pipe { namespace callback {

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
CallbackSendHandler<MsgT,f>::CallbackSendHandler(NodeType const& in_send_node)
  : send_node_(in_send_node)
{ }

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
template <typename SerializerT>
void CallbackSendHandler<MsgT,f>::serialize(SerializerT& s) {
  CallbackBase<SignalBaseType>::serializer(s);
  s | send_node_;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
void CallbackSendHandler<MsgT,f>::trigger_(SignalDataType* data) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    pipe, node,
    "CallbackSendHandler: trigger_: this_node={}, send_node_={}\n",
    this_node, send_node_
  );
  if (this_node == send_node_) {
    f(data);
  } else {
    theMsg()->sendMsg<MsgT,f>(send_node_,data);
  }
}

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_HAN_IMPL_H*/
