
#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/callback_base_tl.h"
#include "vt/pipe/callback/handler_send/callback_send_tl.h"
#include "vt/pipe/msg/callback.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/runnable/general.h"

namespace vt { namespace pipe { namespace callback {

CallbackSendTypeless::CallbackSendTypeless(
  HandlerType const& in_handler, NodeType const& in_send_node
) : send_node_(in_send_node), handler_(in_handler)
{ }

void CallbackSendTypeless::triggerVoid(PipeType const& pipe) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    pipe, node,
    "CallbackSendTypeless: (void) trigger_: pipe={:x}, this_node={}, "
    "send_node_={}\n",
    pipe, this_node, send_node_
  );
  if (this_node == send_node_) {
    runnable::RunnableVoid::run(handler_,this_node);
  } else {
    auto msg = makeSharedMessage<CallbackMsg>(pipe);
    theMsg()->sendMsg<CallbackMsg>(send_node_, handler_, msg);
  }
}

}}} /* end namespace vt::pipe::callback */
