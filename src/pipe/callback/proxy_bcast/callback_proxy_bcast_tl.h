
#if !defined INCLUDED_PIPE_CALLBACK_PROXY_BCAST_CALLBACK_PROXY_BCAST_TL_H
#define INCLUDED_PIPE_CALLBACK_PROXY_BCAST_CALLBACK_PROXY_BCAST_TL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/callback_base_tl.h"
#include "registry/auto/auto_registry_common.h"

namespace vt { namespace pipe { namespace callback {

struct CallbackProxyBcastTypeless : CallbackBaseTL<CallbackProxyBcastTypeless> {
  CallbackProxyBcastTypeless() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s);

  bool operator==(CallbackProxyBcastTypeless const& other) const {
    return true;
  }

public:
  template <typename MsgT>
  void trigger(MsgT* msg, PipeType const& pipe);

  void triggerVoid(PipeType const& pipe) {
    vtAssert(0, "Must not be void");
  }
};

struct CallbackProxyBcastDirect : CallbackBaseTL<CallbackProxyBcastDirect> {
  using AutoHandlerType = auto_registry::AutoHandlerType;

  CallbackProxyBcastDirect() = default;
  CallbackProxyBcastDirect(
    HandlerType const& in_han, AutoHandlerType const& in_vrt,
    bool const& in_member, VirtualProxyType const& in_proxy
  ) : vrt_dispatch_han_(in_vrt), handler_(in_han), proxy_(in_proxy),
      member_(in_member)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s);

  bool operator==(CallbackProxyBcastDirect const& other) const {
    return
      other.handler_ == handler_ &&
      other.vrt_dispatch_han_ == vrt_dispatch_han_ &&
      other.proxy_ == proxy_ &&
      other.member_ == member_;
  }

public:
  template <typename MsgT>
  void trigger(MsgT* msg, PipeType const& pipe);

  void triggerVoid(PipeType const& pipe) {
    vtAssert(0, "Must not be void");
  }

private:
  AutoHandlerType vrt_dispatch_han_ = uninitialized_handler;
  HandlerType handler_              = uninitialized_handler;
  VirtualProxyType proxy_           = no_vrt_proxy;
  bool member_                      = false;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_PROXY_BCAST_CALLBACK_PROXY_BCAST_TL_H*/
