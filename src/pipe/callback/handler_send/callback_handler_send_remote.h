
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_HANDLER_SEND_REMOTE_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_HANDLER_SEND_REMOTE_H

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
  ) : send_node_(in_send_node), handler_(in_handler)
  { }

  HandlerType getHandler() const { return handler_; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    CallbackBase<SignalBaseType>::serializer(s);
    s | send_node_;
    s | handler_;
  }

private:
  void trigger_(SignalDataType* data) override {
    auto const& this_node = theContext()->getNode();
    ::fmt::print(
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

private:
  NodeType send_node_  = uninitialized_destination;
  HandlerType handler_ = uninitialized_handler;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_HANDLER_SEND_REMOTE_H*/
