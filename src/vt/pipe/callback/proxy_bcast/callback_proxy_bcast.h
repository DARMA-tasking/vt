
#if !defined INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_BCAST_H
#define INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_BCAST_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/callback/callback_base.h"
#include "vt/vrt/collection/active/active_funcs.h"
#include "vt/vrt/collection/manager.h"

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

  CallbackProxyBcast(
    HandlerType const& in_handler, ProxyType const& in_proxy,
    bool const& in_member
  ) : proxy_(in_proxy), handler_(in_handler), member_(in_member)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    CallbackBase<SignalBaseType>::serializer(s);
    s | proxy_;
    s | handler_;
    s | member_;
  }

private:
  void trigger_(SignalDataType* data) override {
    theCollection()->broadcastMsgWithHan(
      proxy_,data,handler_,member_,nullptr,true
    );
  }

private:
  ProxyType proxy_     = {};
  HandlerType handler_ = uninitialized_handler;
  bool member_         = false;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_BCAST_H*/
