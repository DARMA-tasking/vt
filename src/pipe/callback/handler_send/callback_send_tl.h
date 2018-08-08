
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_TL_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_TL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/callback_base_tl.h"
#include "activefn/activefn.h"

namespace vt { namespace pipe { namespace callback {

struct CallbackSendTypeless : CallbackBaseTL<CallbackSendTypeless> {
  CallbackSendTypeless() = default;
  CallbackSendTypeless(CallbackSendTypeless const&) = default;
  CallbackSendTypeless(CallbackSendTypeless&&) = default;
  CallbackSendTypeless& operator=(CallbackSendTypeless const&) = default;

  CallbackSendTypeless(
    HandlerType const& in_handler, NodeType const& in_send_node
  );

  template <typename SerializerT>
  void serialize(SerializerT& s);

  HandlerType getHandler() const { return handler_; }
  NodeType getSendNode() const { return send_node_; }

  bool operator==(CallbackSendTypeless const& other) const {
    return other.send_node_ == send_node_ && other.handler_ == handler_;
  }

public:
  template <typename MsgT>
  void trigger(MsgT* msg, PipeType const& pipe);
  void triggerVoid(PipeType const& pipe);

private:
  NodeType send_node_ = uninitialized_destination;
  HandlerType handler_ = uninitialized_handler;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_TL_H*/
