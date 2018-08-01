
#if !defined INCLUDED_PIPE_INTERFACE_CALLBACK_TYPES_H
#define INCLUDED_PIPE_INTERFACE_CALLBACK_TYPES_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/interface/callback_direct.h"
#include "pipe/interface/callback_direct_multi.h"
#include "pipe/callback/handler_send/callback_send.h"
#include "pipe/callback/anon/callback_anon.h"
#include "pipe/callback/handler_bcast/callback_bcast.h"
#include "pipe/signal/signal.h"

namespace vt { namespace pipe { namespace interface {

template <typename T>
struct CallbackTypes {
  using V = signal::SigVoidType;
  using CallbackDirectSend  = CallbackDirect<T,callback::CallbackSend<T>>;
  using CallbackDirectBcast = CallbackDirect<T,callback::CallbackBcast<T>>;
  using CallbackAnon        = CallbackDirect<T,callback::CallbackAnon<T>>;
  using CallbackAnonVoid    = CallbackDirect<V,callback::CallbackAnon<V>>;
};

}}} /* end namespace vt::pipe::interface */

#endif /*INCLUDED_PIPE_INTERFACE_CALLBACK_TYPES_H*/
