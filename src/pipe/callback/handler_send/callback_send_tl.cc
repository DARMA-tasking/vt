
#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/callback_base_tl.h"
#include "pipe/callback/handler_send/callback_send_tl.h"

namespace vt { namespace pipe { namespace callback {

CallbackSendTypeless::CallbackSendTypeless(
  HandlerType const& in_handler, NodeType const& in_send_node
) : send_node_(in_send_node), handler_(in_handler)
{ }

void CallbackSendTypeless::triggerVoid(PipeType const& pipe) {
  assert(0 && "Send: void trigger not allowed");
}

}}} /* end namespace vt::pipe::callback */
