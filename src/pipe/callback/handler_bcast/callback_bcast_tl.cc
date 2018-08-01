
#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/handler_bcast/callback_bcast_tl.h"

#include <cassert>

namespace vt { namespace pipe { namespace callback {

CallbackBcastTypeless::CallbackBcastTypeless(
  HandlerType const& in_handler, bool const& in_include
) : handler_(in_handler), include_sender_(in_include)
{ }

void CallbackBcastTypeless::triggerVoid(PipeType const& pipe) {
  assert(0 && "Bcast: void trigger not allowed");
}

}}} /* end namespace vt::pipe::callback */
