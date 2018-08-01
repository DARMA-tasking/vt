

#include "config.h"
#include "pipe/callback/cb_union/cb_raw_base.h"

namespace vt { namespace pipe { namespace callback { namespace cbunion {

CallbackRawBaseSingle::CallbackRawBaseSingle(
  RawSendMsgTagType, PipeType const& in_pipe, HandlerType const& in_handler,
  NodeType const& in_node
) : pipe_(in_pipe), cb_(SendMsgCB{in_handler,in_node})
{ }

CallbackRawBaseSingle::CallbackRawBaseSingle(
  RawBcastMsgTagType, PipeType const& in_pipe, HandlerType const& in_handler,
  bool const& in_inc
) : pipe_(in_pipe), cb_(BcastMsgCB{in_handler,in_inc})
{ }

CallbackRawBaseSingle::CallbackRawBaseSingle(
  RawAnonTagType, PipeType const& in_pipe
) : pipe_(in_pipe), cb_(AnonCB{})
{ }

void CallbackRawBaseSingle::send() {
  switch (cb_.active_) {
  case CallbackEnum::SendMsgCB:
    cb_.u_.send_msg_cb_.triggerVoid(pipe_);
    break;
  case CallbackEnum::BcastMsgCB:
    cb_.u_.bcast_msg_cb_.triggerVoid(pipe_);
    break;
  case CallbackEnum::AnonCB:
    cb_.u_.anon_cb_.triggerVoid(pipe_);
    break;
  default:
    assert(0 && "Should not be reachable");
  }
}

}}}} /* end namespace vt::pipe::callback::cbunion */
