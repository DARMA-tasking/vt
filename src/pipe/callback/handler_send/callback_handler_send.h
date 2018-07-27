
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_HANDLER_SEND_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_HANDLER_SEND_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/signal/signal.h"
#include "pipe/callback/callback_base.h"
#include "activefn/activefn.h"
#include "context/context.h"
#include "messaging/active.h"

namespace vt { namespace pipe { namespace callback {

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
struct CallbackSendHandler : CallbackBase<signal::Signal<MsgT>> {
  using SignalBaseType = typename signal::Signal<MsgT>;
  using SignalType     = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType = typename SignalType::DataType;

  CallbackSendHandler(NodeType const& in_send_node)
    : send_node_(in_send_node)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    CallbackBase<SignalBaseType>::serializer(s);
    s | send_node_;
  }

private:
  void trigger_(SignalDataType* data) override {
    auto const& this_node = theContext()->getNode();
    if (this_node == send_node_) {
      f(data);
    } else {
      theMsg()->sendMsg<MsgT,f>(send_node_,data);
    }
  }

private:
  NodeType send_node_ = uninitialized_destination;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_HANDLER_SEND_H*/
