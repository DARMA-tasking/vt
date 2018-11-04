
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/callback/callback_base.h"
#include "vt/activefn/activefn.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/messaging/envelope.h"
#include "vt/runnable/general.h"

#include <type_traits>

namespace vt { namespace pipe { namespace callback {

template <typename MsgT>
struct CallbackSend : CallbackBase<signal::Signal<MsgT>> {
  using SignalBaseType = typename signal::Signal<MsgT>;
  using SignalType     = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType = typename SignalType::DataType;
  using MessageType    = MsgT;
  using VoidSigType    = signal::SigVoidType;
  template <typename T, typename U=void>
  using IsVoidType     = std::enable_if_t<std::is_same<T,VoidSigType>::value,U>;
  template <typename T, typename U=void>
  using IsNotVoidType  = std::enable_if_t<!std::is_same<T,VoidSigType>::value,U>;

  CallbackSend() = default;
  CallbackSend(CallbackSend const&) = default;
  CallbackSend(CallbackSend&&) = default;

  CallbackSend(
    HandlerType const& in_handler, NodeType const& in_send_node
  );

  HandlerType getHandler() const { return handler_; }

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
  NodeType send_node_  = uninitialized_destination;
  HandlerType handler_ = uninitialized_handler;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_H*/
