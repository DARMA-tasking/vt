
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_TL_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_TL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/callback_base_tl.h"
#include "activefn/activefn.h"
#include "messaging/active.h"

namespace vt { namespace pipe { namespace callback {

struct CallbackBcastTypeless : CallbackBaseTL<CallbackBcastTypeless> {
  CallbackBcastTypeless() = default;
  CallbackBcastTypeless(CallbackBcastTypeless const&) = default;
  CallbackBcastTypeless(CallbackBcastTypeless&&) = default;
  CallbackBcastTypeless& operator=(CallbackBcastTypeless const&) = default;

  CallbackBcastTypeless(
    HandlerType const& in_handler, bool const& in_include
  );

  HandlerType getHandler() const { return handler_; }
  bool getIncSender() const { return include_sender_; }

  template <typename SerializerT>
  void serialize(SerializerT& s);

public:
  template <typename MsgT>
  void trigger(MsgT* msg, PipeType const& pipe);
  void triggerVoid(PipeType const& pipe);

private:
  HandlerType handler_ = uninitialized_handler;
  bool include_sender_  = false;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_TL_H*/
