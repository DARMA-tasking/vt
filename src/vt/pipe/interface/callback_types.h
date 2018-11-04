
#if !defined INCLUDED_PIPE_INTERFACE_CALLBACK_TYPES_H
#define INCLUDED_PIPE_INTERFACE_CALLBACK_TYPES_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/interface/callback_direct.h"
#include "vt/pipe/interface/callback_direct_multi.h"
#include "vt/pipe/callback/handler_send/callback_send.h"
#include "vt/pipe/callback/anon/callback_anon.h"
#include "vt/pipe/callback/handler_bcast/callback_bcast.h"
#include "vt/pipe/callback/proxy_bcast/callback_proxy_bcast.h"
#include "vt/pipe/callback/proxy_send/callback_proxy_send.h"
#include "vt/pipe/signal/signal.h"

namespace vt { namespace pipe { namespace interface {

template <typename T>
struct CallbackTypes {
  using V = signal::SigVoidType;
  using CallbackDirectSend      = CallbackDirect<T,callback::CallbackSend<T>>;
  using CallbackDirectBcast     = CallbackDirect<T,callback::CallbackBcast<T>>;
  using CallbackDirectVoidSend  = CallbackDirect<V,callback::CallbackSend<V>>;
  using CallbackDirectVoidBcast = CallbackDirect<V,callback::CallbackBcast<V>>;
  using CallbackAnon            = CallbackDirect<T,callback::CallbackAnon<T>>;
  using CallbackAnonVoid        = CallbackDirect<V,callback::CallbackAnon<V>>;
};

template <typename C, typename T>
struct CallbackVrtTypes {
  using CallbackProxyBcast = CallbackDirect<T,callback::CallbackProxyBcast<C,T>>;
  using CallbackProxySend  = CallbackDirect<T,callback::CallbackProxySend<C,T>>;
};

}}} /* end namespace vt::pipe::interface */

#endif /*INCLUDED_PIPE_INTERFACE_CALLBACK_TYPES_H*/
