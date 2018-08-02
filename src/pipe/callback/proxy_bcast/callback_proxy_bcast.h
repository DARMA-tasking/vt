
#if !defined INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_BCAST_H
#define INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_BCAST_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/signal/signal.h"
#include "pipe/callback/callback_base.h"
#include "vrt/collection/active/active_funcs.h"
#include "vrt/collection/manager.h"

#include <functional>
#include <cassert>

namespace vt { namespace pipe { namespace callback {

template <typename ColT, typename MsgT>
struct CallbackProxyBcast : CallbackBase<signal::Signal<MsgT>> {
  using SignalBaseType  = typename signal::Signal<MsgT>;
  using SignalType      = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType  = typename SignalType::DataType;
  using ProxyType       = typename ColT::CollectionProxyType;
  using MessageType     = MsgT;

  CallbackProxyBcast() = default;
  CallbackProxyBcast(CallbackProxyBcast const&) = default;
  CallbackProxyBcast(CallbackProxyBcast&&) = default;

  CallbackProxyBcast(HandlerType const& in_handler, ProxyType const& in_proxy)
    : proxy_(in_proxy), handler_(in_handler)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    CallbackBase<SignalBaseType>::serializer(s);
    s | proxy_;
    s | handler_;
  }

private:
  void trigger_(SignalDataType* data) override {
    theCollection()->broadcastMsgUntypedHandler(
      proxy_,data,handler_,false,nullptr,true
    );
  }

private:
  ProxyType proxy_     = {};
  HandlerType handler_ = uninitialized_handler;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_BCAST_H*/
