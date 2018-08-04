
#if !defined INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_H
#define INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/callback_base_tl.h"

namespace vt { namespace pipe { namespace callback {

struct CallbackProxySendTypeless : CallbackBaseTL<CallbackProxySendTypeless> {
  CallbackProxySendTypeless() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s);

public:
  template <typename MsgT>
  void trigger(MsgT* msg, PipeType const& pipe);

  void triggerVoid(PipeType const& pipe) {
    assert(0 && "Must not be void");
  }
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_H*/
