
#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/handler_bcast/callback_bcast_tl.h"

namespace vt { namespace pipe { namespace callback {

CallbackBcastTypeless::CallbackBcastTypeless(
  HandlerType const& in_handler, bool const& in_include
) : handler_(in_handler), include_sender_(in_include)
{ }

}}} /* end namespace vt::pipe::callback */
