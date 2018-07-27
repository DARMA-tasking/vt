
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_HANDLER_SEND_REMOTE_TL_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_HANDLER_SEND_REMOTE_TL_H

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

struct CallbackSendTypeless : CallbackBase<signal::SignalVoid> {
  using SignalBaseType = typename signal::SignalVoid;

  CallbackSendTypeless(
    HandlerType const& in_handler, NodeType const& in_send_node
  ) : send_node_(in_send_node), handler_(in_handler)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    CallbackBase<SignalBaseType>::serializer(s);
    s | send_node_;
    s | handler_;
  }

  HandlerType getHandler() const { return handler_; }

private:
  template <typename MsgT>
  void triggerTypeless(MsgT* msg) {
    auto const& this_node = theContext()->getNode();
    if (this_node == send_node_) {
      runnable::Runnable<ShortMessage>::run(handler_, nullptr, msg, this_node);
    } else {
      return theMsg()->sendMsg<MsgT>(send_node_, handler_, msg);
    }
  }

private:
  NodeType send_node_ = uninitialized_destination;
  HandlerType handler_ = uninitialized_handler;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_HANDLER_SEND_REMOTE_TL_H*/
