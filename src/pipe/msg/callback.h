
#if !defined INCLUDED_PIPE_MSG_CALLBACK_H
#define INCLUDED_PIPE_MSG_CALLBACK_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "messaging/message.h"

namespace vt { namespace pipe {

struct CallbackMsg : ::vt::Message {

  CallbackMsg() = default;

  explicit CallbackMsg(PipeType const& in_pipe)
    : pipe_(in_pipe)
  { }
  CallbackMsg(PipeType const& in_pipe, bool const& in_typeless)
    : pipe_(in_pipe), typeless_(in_typeless)
  { }

  PipeType getPipe() const { return pipe_; }
  bool typeless() const { return typeless_; }

private:
  PipeType pipe_ = no_pipe_op;
  bool typeless_ = false;
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_MSG_CALLBACK_H*/
