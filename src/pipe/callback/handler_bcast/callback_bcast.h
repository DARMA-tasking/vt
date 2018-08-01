
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_H

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
struct CallbackBcast : CallbackBase<signal::Signal<MsgT>> {
  using SignalBaseType = typename signal::Signal<MsgT>;
  using SignalType     = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType = typename SignalType::DataType;
  using MessageType    = MsgT;

  CallbackBcast() = default;
  CallbackBcast(CallbackBcast const&) = default;
  CallbackBcast(CallbackBcast&&) = default;

  CallbackBcast(
    HandlerType const& in_handler, bool const& in_include
  ) : handler_(in_handler), include_sender_(in_include)
  { }

  HandlerType getHandler() const { return handler_; }
  bool getIncSender() const { return include_sender_; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    CallbackBase<SignalBaseType>::serializer(s);
    s | include_sender_;
    s | handler_;
  }

private:
  void trigger_(SignalDataType* data) override {
    auto const& this_node = theContext()->getNode();
    debug_print(
      pipe, node,
      "CallbackBcast: trigger_: this_node={}, include_sender_={}\n",
      this_node, include_sender_
    );
    theMsg()->broadcastMsg<SignalDataType>(handler_, data);
    if (include_sender_) {
      auto nmsg = makeSharedMessage<SignalDataType>(*data);
      auto nmsgc = reinterpret_cast<ShortMessage*>(nmsg);
      messageRef(nmsg);
      runnable::Runnable<ShortMessage>::run(handler_,nullptr,nmsgc,this_node);
      messageDeref(nmsg);
    }
  }

private:
  HandlerType handler_ = uninitialized_handler;
  bool include_sender_  = false;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_H*/
