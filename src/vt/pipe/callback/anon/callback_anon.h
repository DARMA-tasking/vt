
#if !defined INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_H
#define INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/msg/callback.h"
#include "vt/pipe/callback/callback_base.h"

#include <functional>
#include <cassert>
#include <type_traits>

namespace vt { namespace pipe { namespace callback {

template <typename MsgT>
struct CallbackAnon : CallbackBase<signal::Signal<MsgT>> {
  using SignalBaseType = typename signal::Signal<MsgT>;
  using SignalType     = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType = typename SignalType::DataType;
  using CallbackFnType = std::function<void(SignalDataType)>;
  using VoidSigType    = signal::SigVoidType;
  template <typename T, typename U=void>
  using IsVoidType     = std::enable_if_t<std::is_same<T,VoidSigType>::value,U>;
  template <typename T, typename U=void>
  using IsNotVoidType  = std::enable_if_t<!std::is_same<T,VoidSigType>::value,U>;

  CallbackAnon() = default;
  CallbackAnon(CallbackAnon const&) = default;
  CallbackAnon(CallbackAnon&&) = default;

  template <typename SerializerT>
  void serialize(SerializerT& s);

private:
  template <typename T>
  IsVoidType<T> triggerDispatch(SignalDataType* data, PipeType const& pid);

  template <typename T>
  IsNotVoidType<T> triggerDispatch(SignalDataType* data, PipeType const& pid);

  void trigger_(SignalDataType* data, PipeType const& pid) override;
  void trigger_(SignalDataType* data) override;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_H*/
