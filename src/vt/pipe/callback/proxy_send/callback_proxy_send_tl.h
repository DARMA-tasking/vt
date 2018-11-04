
#if !defined INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_H
#define INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/callback_base_tl.h"
#include "vt/registry/auto/auto_registry_common.h"

namespace vt { namespace pipe { namespace callback {

struct CallbackProxySendTypeless : CallbackBaseTL<CallbackProxySendTypeless> {
  CallbackProxySendTypeless() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s);

  bool operator==(CallbackProxySendTypeless const& other) const {
    return true;
  }

public:
  template <typename MsgT>
  void trigger(MsgT* msg, PipeType const& pipe);

  void triggerVoid(PipeType const& pipe) {
    vtAssert(0, "Must not be void");
  }
};

struct CallbackProxySendDirect : CallbackBaseTL<CallbackProxySendDirect> {
  using AutoHandlerType = auto_registry::AutoHandlerType;

  CallbackProxySendDirect() = default;
  CallbackProxySendDirect(HandlerType const& in_han, AutoHandlerType in_vrt)
    : vrt_dispatch_han_(in_vrt), handler_(in_han)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s);

  bool operator==(CallbackProxySendDirect const& other) const {
    return
      other.handler_ == handler_ &&
      other.vrt_dispatch_han_ == vrt_dispatch_han_;
  }

public:
  template <typename MsgT>
  void trigger(MsgT* msg, PipeType const& pipe);

  void triggerVoid(PipeType const& pipe) {
    vtAssert(0, "Must not be void");
  }

private:
  AutoHandlerType vrt_dispatch_han_ = uninitialized_handler;
  HandlerType handler_ = uninitialized_handler;
};


}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_H*/
