
#if !defined INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_IMPL_H
#define INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/anon/callback_anon.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/msg/callback.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/messaging/active.h"
#include "vt/messaging/envelope.h"
#include "vt/context/context.h"

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
  auto const& this_node = theContext()->getNode();
  auto const& pipe_node = PipeIDBuilder::getNode(pid);
  debug_print(
    pipe, node,
    "CallbackAnon: (T signal) trigger_: pipe_={:x}, pipe_node={}\n",
    pid, pipe_node
  );
  if (this_node == pipe_node) {
    theCB()->triggerPipeTyped<T>(pid,data);
  } else {
    /*
     * Set pipe type on the message envelope; use the group in the envelope in
     * indicate the pipe
     */
    setPipeType(data->env);
    envelopeSetGroup(data->env,pid);
    theMsg()->sendMsgAuto<T,PipeManager::triggerCallbackMsgHan>(pipe_node, data);
  }
}

template <typename MsgT>
void CallbackAnon<MsgT>::trigger_(SignalDataType* data, PipeType const& pid) {
  triggerDispatch<MsgT>(data,pid);
}

template <typename MsgT>
void CallbackAnon<MsgT>::trigger_(SignalDataType* data) {
  vtAssert(0, "Should not be reachable in this derived class");
}

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_IMPL_H*/
