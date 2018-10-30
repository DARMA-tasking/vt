
#if !defined INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_TL_H
#define INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_TL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/signal/signal.h"
#include "pipe/msg/callback.h"
#include "pipe/callback/callback_base_tl.h"

#include <functional>
#include <type_traits>

namespace vt { namespace pipe { namespace callback {

struct CallbackAnonTypeless : CallbackBaseTL<CallbackAnonTypeless> {
  CallbackAnonTypeless() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s);

  bool operator==(CallbackAnonTypeless const& other) const {
    return true;
  }

public:
  template <typename MsgT>
  void trigger(MsgT* msg, PipeType const& pipe);
  void triggerVoid(PipeType const& pipe);
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_TL_H*/
