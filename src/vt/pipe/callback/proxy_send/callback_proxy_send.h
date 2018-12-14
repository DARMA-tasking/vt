
#if !defined INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_SEND_H
#define INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_SEND_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/callback/callback_base.h"
#include "vt/vrt/collection/active/active_funcs.h"
#include "vt/vrt/proxy/collection_elm_proxy.h"
#include "vt/vrt/collection/manager.h"

#include <cassert>

namespace vt { namespace pipe { namespace callback {

template <typename ColT, typename MsgT>
struct CallbackProxySend : CallbackBase<signal::Signal<MsgT>> {
  using SignalBaseType   = typename signal::Signal<MsgT>;
  using SignalType       = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType   = typename SignalType::DataType;
  using IndexedProxyType = typename ColT::ProxyType;
  using ProxyType        = typename ColT::CollectionProxyType;
  using IndexType        = typename ColT::IndexType;
  using MessageType      = MsgT;

  CallbackProxySend(
    HandlerType const& in_handler, IndexedProxyType const& in_proxy,
    bool const& in_member
  ) : proxy_(in_proxy.getCollectionProxy()),
      idx_(in_proxy.getElementProxy().getIndex()),
      handler_(in_handler), member_(in_member)
  { }

  CallbackProxySend(
    HandlerType const& in_handler, ProxyType const& in_proxy,
    IndexType const& in_idx, bool const& in_member
  ) : proxy_(in_proxy), idx_(in_idx), handler_(in_handler), member_(in_member)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    CallbackBase<SignalBaseType>::serializer(s);
    s | proxy_ | idx_;
    s | handler_;
    s | member_;
  }

private:
  void trigger_(SignalDataType* data) override {
    theCollection()->sendMsgWithHan(
      proxy_.index(idx_),data,handler_,member_,nullptr
    );
  }

private:
  ProxyType proxy_     = {};
  IndexType idx_       = {};
  HandlerType handler_ = uninitialized_handler;
  bool member_         = false;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_SEND_H*/
