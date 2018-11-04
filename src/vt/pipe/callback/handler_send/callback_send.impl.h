
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_IMPL_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/handler_send/callback_send.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/runnable/general.h"

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
void CallbackSend<MsgT>::trigger_(SignalDataType* data, PipeType const& pid) {
  triggerDispatch<MsgT>(data,pid);
}

template <typename MsgT>
void CallbackSend<MsgT>::trigger_(SignalDataType* data) {
  assert(0 && "Should not be reachable in this derived class");
}

template <typename MsgT>
template <typename T>
CallbackSend<MsgT>::IsVoidType<T>
CallbackSend<MsgT>::triggerDispatch(SignalDataType* data, PipeType const& pid) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    pipe, node,
    "CallbackSend: (void) trigger_: this_node={}, send_node_={}\n",
    this_node, send_node_
  );
  if (this_node == send_node_) {
    runnable::RunnableVoid::run(handler_,this_node);
  } else {
    auto msg = makeSharedMessage<CallbackMsg>(pid);
    theMsg()->sendMsg<CallbackMsg>(send_node_, handler_, msg);
  }
}

template <typename MsgT>
template <typename T>
CallbackSend<MsgT>::IsNotVoidType<T>
CallbackSend<MsgT>::triggerDispatch(SignalDataType* data, PipeType const& pid) {
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
    theMsg()->sendMsgAuto<SignalDataType>(send_node_, handler_, data);
  }
}


}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_IMPL_H*/
