
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

  PipeType getPipe() const { return pipe_; }

private:
  PipeType pipe_ = no_pipe_op;
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_MSG_CALLBACK_H*/
