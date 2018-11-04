
#if !defined INCLUDED_PIPE_CALLBACK_CALLBACK_BASE_TL_H
#define INCLUDED_PIPE_CALLBACK_CALLBACK_BASE_TL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"

namespace vt { namespace pipe { namespace callback {

template <typename CallbackT>
struct CallbackBaseTL {
  CallbackBaseTL() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s) { }

  template <typename MsgT>
  void trigger(MsgT* msg, PipeType const& pipe) {
    auto cb = static_cast<CallbackT&>(*this);
    return cb.template trigger<MsgT>(msg,pipe);
  }

  void triggerVoid(PipeType const& pipe) {
    auto cb = static_cast<CallbackT&>(*this);
    return cb.template triggerVoid(pipe);
  }
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_CALLBACK_BASE_TL_H*/
