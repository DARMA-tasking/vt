
#if !defined INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_LISTENER_IMPL_H
#define INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_LISTENER_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/callback/callback_base.h"
#include "vt/pipe/callback/anon/callback_anon_listener.h"

#include <functional>
#include <cassert>

namespace vt { namespace pipe { namespace callback {

template <typename SignalT>
AnonListener<SignalT>::AnonListener(
  CallbackFnType const& in_fn, bool is_persist, RefType refs
) : fn_(in_fn),
    CallbackBase<SignalT>(CallbackExplicitTag, is_persist, refs)
{ }

template <typename SignalT>
AnonListener<SignalT>::AnonListener(CallbackFnType const& in_fn)
  : fn_(in_fn),
    CallbackBase<SignalT>(CallbackSingleUseTag)
{ }

template <typename SignalT>
void AnonListener<SignalT>::trigger_(SignalDataType* data) {
  assert(fn_ != nullptr && "Must have valid function pointer");
  fn_(data);
}

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_LISTENER_IMPL_H*/
