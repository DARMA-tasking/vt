
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_TL_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_TL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/callback_base_tl.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/active.h"

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

  bool operator==(CallbackBcastTypeless const& other) const {
    return
      other.include_sender_ == include_sender_ &&
      other.handler_ == handler_;
  }

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
