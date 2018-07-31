
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/signal/signal.h"
#include "pipe/callback/callback_base.h"
#include "activefn/activefn.h"
#include "context/context.h"
#include "messaging/active.h"
#include "messaging/envelope.h"
#include "runnable/general.h"

namespace vt { namespace pipe { namespace callback {

template <typename MsgT>
struct CallbackSend : CallbackBase<signal::Signal<MsgT>> {
  using SignalBaseType = typename signal::Signal<MsgT>;
  using SignalType     = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType = typename SignalType::DataType;
  using MessageType    = MsgT;

  CallbackSend() = default;
  CallbackSend(CallbackSend const&) = default;
  CallbackSend(CallbackSend&&) = default;

  CallbackSend(
    HandlerType const& in_handler, NodeType const& in_send_node
  );

  HandlerType getHandler() const { return handler_; }

  template <typename SerializerT>
  void serialize(SerializerT& s);

private:
  void trigger_(SignalDataType* data) override;

private:
  NodeType send_node_  = uninitialized_destination;
  HandlerType handler_ = uninitialized_handler;
};

}}} /* end namespace vt::pipe::callback */

#include "pipe/callback/handler_send/callback_send.impl.h"

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_H*/
