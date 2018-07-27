
#if !defined INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_BCAST_H
#define INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_BCAST_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/signal/signal.h"
#include "pipe/callback/callback_base.h"
#include "vrt/collection/active/active_funcs.h"

#include <functional>
#include <cassert>

namespace vt { namespace pipe { namespace callback {

template <
  typename ColT,
  typename MsgT,
  vrt::collection::ActiveColTypedFnType<MsgT,ColT>* f
>
struct CallbackProxyBcast : CallbackBase<signal::Signal<MsgT>> {
  using SignalBaseType  = typename signal::Signal<MsgT>;
  using SignalType      = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType  = typename SignalType::DataType;
  using ProxyType       = typename ColT::CollectionProxyType;
  using CallbackFnType  = std::function<void(SignalDataType)>;
  using MessageType     = MsgT;

  explicit CallbackProxyBcast(ProxyType const& in_proxy)
    : proxy_(in_proxy)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    CallbackBase<SignalBaseType>::serializer(s);
    s | proxy_;
  }

private:
  void trigger_(SignalDataType* data) override {
    proxy_.template broadcast<ColT,MsgT,f>(data);
  }

private:
  ProxyType proxy_ = {};
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_BCAST_H*/
