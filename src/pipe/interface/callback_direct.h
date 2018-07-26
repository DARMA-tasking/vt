
#if !defined INCLUDED_PIPE_INTERFACE_CALLBACK_DIRECT_H
#define INCLUDED_PIPE_INTERFACE_CALLBACK_DIRECT_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/id/pipe_id.h"
#include "pipe/interface/base_container.h"
#include "pipe/interface/send_container.h"
#include "pipe/callback/handler_send/callback_handler_send.h"

namespace vt { namespace pipe { namespace interface {

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
struct CallbackDirectSend : SendContainer<MsgT*,callback::CallbackSend<MsgT,f>> {

  CallbackDirectSend(PipeType const& in_pipe, NodeType const& in_node)
    : SendContainer<MsgT*,callback::CallbackSend<MsgT,f>>(
        in_pipe, callback::CallbackSend<MsgT,f>(in_node)
      )
  { }

  void send(MsgT* m) {
    SendContainer<MsgT*,callback::CallbackSend<MsgT,f>>::trigger(m);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    SendContainer<MsgT*,callback::CallbackSend<MsgT,f>>::serialize(s);
  }

};

}}} /* end namespace vt::pipe::interface */

#endif /*INCLUDED_PIPE_INTERFACE_CALLBACK_DIRECT_H*/
