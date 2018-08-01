
#if !defined INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_IMPL_H
#define INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_IMPL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/anon/callback_anon.h"
#include "pipe/signal/signal.h"
#include "pipe/msg/callback.h"
#include "pipe/pipe_manager.h"
#include "messaging/active.h"
#include "context/context.h"

#include <cassert>

namespace vt { namespace pipe { namespace callback {

template <typename MsgT>
template <typename SerializerT>
void CallbackAnon<MsgT>::serialize(SerializerT& s) {
  CallbackBase<MsgT>::serialize(s);
}

template <typename MsgT>
template <typename T>
CallbackAnon<MsgT>::IsVoidType<T>
CallbackAnon<MsgT>::triggerDispatch(SignalDataType* data, PipeType const& pid) {
  // Overload when the signal is void
  auto const& this_node = theContext()->getNode();
  auto const& pipe_node = PipeIDBuilder::getNode(pid);
  debug_print(
    pipe, node,
    "CallbackAnon: (void signal) trigger_: pipe_={:x}, pipe_node={}\n",
    pid, pipe_node
  );
  if (this_node == pipe_node) {
    theCB()->triggerPipe(pid);
  } else {
    auto msg = makeSharedMessage<CallbackMsg>(pid);
    theMsg()->sendMsg<CallbackMsg,PipeManager::triggerCallbackHan>(
      pipe_node, msg
    );
  }
}

template <typename MsgT>
template <typename T>
CallbackAnon<MsgT>::IsNotVoidType<T>
CallbackAnon<MsgT>::triggerDispatch(SignalDataType* data, PipeType const& pid) {
  // Overload when the signal is non-void
  assert(0);
}

template <typename MsgT>
void CallbackAnon<MsgT>::trigger_(SignalDataType* data, PipeType const& pid) {
  triggerDispatch<MsgT>(data,pid);
}

template <typename MsgT>
void CallbackAnon<MsgT>::trigger_(SignalDataType* data) {
  assert(0 && "Should not be reachable in this derived class");
}

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_IMPL_H*/
