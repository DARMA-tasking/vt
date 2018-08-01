
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

}}}} /* end namespace vt::pipe::callback::cbunion */
