
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/signal/signal.h"
#include "pipe/callback/callback_base.h"
#include "activefn/activefn.h"
#include "context/context.h"
#include "messaging/active.h"
#include "messaging/envelope.h"
#include "runnable/general.h"

namespace vt { namespace pipe { namespace callback {

template <typename MsgT>
struct CallbackBcast : CallbackBase<signal::Signal<MsgT>> {
  using SignalBaseType = typename signal::Signal<MsgT>;
  using SignalType     = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType = typename SignalType::DataType;
  using MessageType    = MsgT;
  using VoidSigType    = signal::SigVoidType;
  template <typename T, typename U=void>
  using IsVoidType     = std::enable_if_t<std::is_same<T,VoidSigType>::value,U>;
  template <typename T, typename U=void>
  using IsNotVoidType  = std::enable_if_t<!std::is_same<T,VoidSigType>::value,U>;

  CallbackBcast() = default;
  CallbackBcast(CallbackBcast const&) = default;
  CallbackBcast(CallbackBcast&&) = default;

  CallbackBcast(
    HandlerType const& in_handler, bool const& in_include
  );

  HandlerType getHandler() const { return handler_; }
  bool getIncSender() const { return include_sender_; }

  template <typename SerializerT>
  void serialize(SerializerT& s);

private:
  template <typename T>
  IsVoidType<T> triggerDispatch(SignalDataType* data, PipeType const& pid);

  template <typename T>
  IsNotVoidType<T> triggerDispatch(SignalDataType* data, PipeType const& pid);

  void trigger_(SignalDataType* data, PipeType const& pid) override;
  void trigger_(SignalDataType* data) override;

private:
  HandlerType handler_ = uninitialized_handler;
  bool include_sender_  = false;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_H*/
