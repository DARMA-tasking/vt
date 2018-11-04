
#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/handler_bcast/callback_bcast_tl.h"
#include "vt/pipe/msg/callback.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/runnable/general.h"

#include <cassert>

namespace vt { namespace pipe { namespace callback {

CallbackBcastTypeless::CallbackBcastTypeless(
  HandlerType const& in_handler, bool const& in_include
) : handler_(in_handler), include_sender_(in_include)
{ }

void CallbackBcastTypeless::triggerVoid(PipeType const& pipe) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    pipe, node,
    "CallbackBcast: (void) trigger_: pipe={:x}, this_node={}, "
    "include_sender_={}\n",
    pipe, this_node, include_sender_
  );
  auto msg = makeSharedMessage<CallbackMsg>(pipe);
  theMsg()->broadcastMsg<CallbackMsg>(handler_,msg);
  if (include_sender_) {
    runnable::RunnableVoid::run(handler_,this_node);
  }
}

}}} /* end namespace vt::pipe::callback */
