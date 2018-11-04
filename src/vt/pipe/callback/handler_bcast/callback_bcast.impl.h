
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_IMPL_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/handler_bcast/callback_bcast.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/runnable/general.h"

#include <cassert>

namespace vt { namespace pipe { namespace callback {

template <typename MsgT>
CallbackBcast<MsgT>::CallbackBcast(
  HandlerType const& in_handler, bool const& in_include
) : handler_(in_handler), include_sender_(in_include)
{ }

template <typename MsgT>
template <typename SerializerT>
void CallbackBcast<MsgT>::serialize(SerializerT& s) {
  CallbackBase<SignalBaseType>::serializer(s);
  s | include_sender_;
  s | handler_;
}

template <typename MsgT>
void CallbackBcast<MsgT>::trigger_(SignalDataType* data, PipeType const& pid) {
  triggerDispatch<MsgT>(data,pid);
}

template <typename MsgT>
void CallbackBcast<MsgT>::trigger_(SignalDataType* data) {
  assert(0 && "Should not be reachable in this derived class");
}

template <typename MsgT>
template <typename T>
CallbackBcast<MsgT>::IsVoidType<T>
CallbackBcast<MsgT>::triggerDispatch(SignalDataType* data, PipeType const& pid) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    pipe, node,
    "CallbackBcast: (void) trigger_: this_node={}, include_sender_={}\n",
    this_node, include_sender_
  );
  auto msg = makeSharedMessage<CallbackMsg>(pid);
  theMsg()->broadcastMsg<CallbackMsg>(handler_,msg);
  if (include_sender_) {
    runnable::RunnableVoid::run(handler_,this_node);
  }
}

template <typename MsgT>
template <typename T>
CallbackBcast<MsgT>::IsNotVoidType<T>
CallbackBcast<MsgT>::triggerDispatch(SignalDataType* data, PipeType const& pid) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    pipe, node,
    "CallbackBcast: trigger_: this_node={}, include_sender_={}\n",
    this_node, include_sender_
  );
  theMsg()->broadcastMsgAuto<SignalDataType>(handler_, data);
  if (include_sender_) {
    auto nmsg = makeSharedMessage<SignalDataType>(*data);
    auto nmsgc = reinterpret_cast<ShortMessage*>(nmsg);
    messageRef(nmsg);
    runnable::Runnable<ShortMessage>::run(handler_,nullptr,nmsgc,this_node);
    messageDeref(nmsg);
  }
}

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_IMPL_H*/
