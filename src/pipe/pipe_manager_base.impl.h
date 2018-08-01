
#if !defined INCLUDED_PIPE_PIPE_MANAGER_BASE_IMPL_H
#define INCLUDED_PIPE_PIPE_MANAGER_BASE_IMPL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/pipe_manager_base.h"
#include "pipe/pipe_manager.h"
#include "pipe/state/pipe_state.h"
#include "pipe/interface/remote_container_msg.h"
#include "pipe/interface/send_container.h"
#include "pipe/interface/callback_direct.h"
#include "pipe/signal/signal_holder.h"
#include "pipe/id/pipe_id.h"
#include "pipe/callback/anon/callback_anon_listener.h"
#include "context/context.h"
#include "messaging/envelope.h"
#include "registry/auto/auto_registry.h"

#include <cassert>
#include <tuple>
#include <type_traits>
#include <array>

namespace vt { namespace pipe {

template <typename SignalT>
/*static*/ signal::SignalHolder<SignalT> PipeManagerBase::signal_holder_ = {};

template <typename MsgT>
/*static*/ void PipeManagerBase::triggerCallbackMsgHan(MsgT* msg) {
  auto const& is_pipe = messaging::envelopeIsPipe(msg->env);
  assert(is_pipe && "Must be pipe type");
  auto const& pipe_id = envelopeGetGroup(msg->env);
  assert(pipe_id != no_group && pipe_id != no_pipe && "Pipe must be valid");
  theCB()->triggerPipeTyped<MsgT>(pipe_id,msg);
}

template <typename MsgT>
void PipeManagerBase::triggerPipeTyped(PipeType const& pipe, MsgT* msg) {
  using SignalType = signal::Signal<MsgT>;
  signal_holder_<SignalType>.deliverAll(pipe,msg);
  generalSignalTrigger(pipe);
}

template <typename SignalT, typename ListenerT>
void PipeManagerBase::registerCallback(
  PipeType const& pipe, ListenerT&& listener, bool update_state
) {
  /*
   *  Save the new listener in the typed signal holder container for delivery
   *  when a signal arrives
   */
  signal_holder_<SignalT>.addListener(pipe,std::move(listener));
  /*
   *  Update the number of registered listeners in the pipe state
   */
  if (update_state) {
    auto iter = pipe_state_.find(pipe);
    assert(iter != pipe_state_.end() && "Pipe state must exist");
    iter->second.listenerReg();
  }
}

template <typename MsgT>
PipeType PipeManagerBase::makeCallbackFunc(
  bool const& persist, FuncMsgType<MsgT> fn, RefType num_signals,
  RefType num_listeners
) {
  bool const& send_back = true;
  auto const& pipe_id = makePipeID(persist,send_back);
  /*
   *  Add a new entry for pipe state to track it on this node
   */
  newPipeState(pipe_id,persist,num_signals,num_listeners,1);

  /*
   *  Register a listener for this signal to trigger the anon function when the
   *  callback is invoked
   */
  using SignalType = signal::Signal<MsgT>;
  assert(num_listeners > 0 && "Number of listeners must be positive");
  auto const& num_refs = !persist ? num_signals / num_listeners : -1;
  auto anon = std::make_unique<callback::AnonListener<SignalType>>(
    fn, persist, num_refs
  );
  /*
   *  Do not update the pipe state since this newly registered listener is
   *  already counted by the `1' in the newPipeState(...,1)
   */
  registerCallback<SignalType>(pipe_id, std::move(anon), false);
  return pipe_id;
}

template <typename MsgT>
void PipeManagerBase::addListener(PipeType const& pipe, FuncMsgType<MsgT> fn) {
  using SignalType = signal::Signal<MsgT>;
  auto iter = pipe_state_.find(pipe);
  assert(iter != pipe_state_.end() && "Pipe state must exist");
  auto const& state = iter->second;
  auto const& persist = state.isPersist();
  auto const& refs = state.refsPerListener();
  return registerCallback<SignalType>(
    pipe, std::make_unique<callback::AnonListener<SignalType>>(fn,persist,refs)
  );
}

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_BASE_IMPL_H*/
