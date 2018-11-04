
#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_HAN_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_HAN_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/callback/callback_base.h"
#include "vt/activefn/activefn.h"

namespace vt { namespace pipe { namespace callback {

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
struct CallbackSendHandler : CallbackBase<signal::Signal<MsgT>> {
  using SignalBaseType = typename signal::Signal<MsgT>;
  using SignalType     = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType = typename SignalType::DataType;
  using MessageType    = MsgT;

  CallbackSendHandler(NodeType const& in_send_node);

  template <typename SerializerT>
  void serialize(SerializerT& s);

private:
  void trigger_(SignalDataType* data) override;

private:
  NodeType send_node_ = uninitialized_destination;
};

}}} /* end namespace vt::pipe::callback */

#include "vt/pipe/callback/handler_send/callback_send_han.impl.h"

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_HAN_H*/
