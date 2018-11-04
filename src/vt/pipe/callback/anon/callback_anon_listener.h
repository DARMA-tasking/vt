
#if !defined INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_LISTENER_H
#define INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_LISTENER_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/callback/callback_base.h"

#include <functional>
#include <cassert>

namespace vt { namespace pipe { namespace callback {

template <typename SignalT>
struct AnonListener : CallbackBase<SignalT> {
  using SignalType     = SignalT;
  using SignalDataType = typename SignalType::DataType;
  using CallbackFnType = std::function<void(SignalDataType*)>;

  AnonListener() = default;
  AnonListener(AnonListener const&) = default;
  AnonListener(AnonListener&&) = default;
  AnonListener(CallbackFnType const& in_fn, bool is_persist, RefType refs = -1);
  explicit AnonListener(CallbackFnType const& in_fn);

private:
  void trigger_(SignalDataType* data) override;

private:
  CallbackFnType fn_ = nullptr;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_LISTENER_H*/
