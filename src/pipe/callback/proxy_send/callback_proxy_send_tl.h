
#if !defined INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_H
#define INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/callback_base_tl.h"
#include "registry/auto/auto_registry_common.h"

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

struct CallbackProxySendDirect : CallbackBaseTL<CallbackProxySendDirect> {
  using AutoHandlerType = auto_registry::AutoHandlerType;

  CallbackProxySendDirect() = default;
  CallbackProxySendDirect(HandlerType const& in_han, AutoHandlerType in_vrt)
    : vrt_dispatch_han_(in_vrt), handler_(in_han)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s);

public:
  template <typename MsgT>
  void trigger(MsgT* msg, PipeType const& pipe);

  void triggerVoid(PipeType const& pipe) {
    assert(0 && "Must not be void");
  }

private:
  AutoHandlerType vrt_dispatch_han_ = uninitialized_handler;
  HandlerType handler_ = uninitialized_handler;
};


}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_H*/
