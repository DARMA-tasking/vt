/*
//@HEADER
// *****************************************************************************
//
//                           pipe_manager_base.impl.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_PIPE_PIPE_MANAGER_BASE_IMPL_H
#define INCLUDED_PIPE_PIPE_MANAGER_BASE_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager_base.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/pipe/state/pipe_state.h"
#include "vt/pipe/interface/remote_container.h"
#include "vt/pipe/interface/send_container.h"
#include "vt/pipe/interface/callback_direct.h"
#include "vt/pipe/signal/signal_holder.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/pipe/callback/anon/callback_anon_listener.h"
#include "vt/context/context.h"
#include "vt/messaging/envelope.h"
#include "vt/registry/auto/auto_registry.h"

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
  vtAssert(is_pipe, "Must be pipe type");
  auto const& pipe_id = envelopeGetGroup(msg->env);
  vtAssert(pipe_id != no_group && pipe_id != no_pipe, "Pipe must be valid");
  theCB()->triggerPipeTyped<MsgT>(pipe_id,msg);
}

template <typename MsgT>
void PipeManagerBase::triggerPipeTyped(PipeType const& pipe, MsgT* msg) {
  using SignalType = signal::Signal<MsgT>;
  auto const& exists = signal_holder_<SignalType>.exists(pipe);
  if (exists) {
    signal_holder_<SignalType>.deliverAll(pipe,msg);
  } else {
    auto nmsg = makeMessage<MsgT>(*msg);
    triggerPipeUnknown<MsgT>(pipe,nmsg.get());
  }
  generalSignalTrigger(pipe);
}

template <typename MsgT>
void PipeManagerBase::triggerPipeUnknown(PipeType const& pipe, MsgT* msg) {
  auto iter = pipe_state_.find(pipe);
  vtAssert(iter != pipe_state_.end(), "Pipe state must exist");

  debug_print(
    pipe, node,
    "PipeManagerBase: triggerPipeUnknown: pipe={:x}, msg_ptr={}: "
    "dynamic dispatch\n",
    pipe, print_ptr(msg)
  );

  iter->second.dispatch(msg);
}

template <typename SignalT, typename ListenerT>
void PipeManagerBase::registerCallback(
  PipeType const& pipe, ListenerT&& listener, bool update_state
) {
  auto& holder = signal_holder_<SignalT>;
  auto const holder_id = holder.getID();

  auto cleanup_iter = signal_cleanup_fns_.find(holder_id);
  if (cleanup_iter == signal_cleanup_fns_.end()) {
    signal_cleanup_fns_[holder_id] = []{
      signal_holder_<SignalT>.clearAll();
    };
  }

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
    vtAssert(iter != pipe_state_.end(), "Pipe state must exist");
    iter->second.listenerReg();
  }
}

template <typename MsgT, typename ListenerT>
PipeType PipeManagerBase::makeCallbackAny(
  bool const& persist, ListenerT&& listener, bool const& dispatch,
  RefType num_signals, RefType num_listeners
) {
  using SignalType = signal::Signal<MsgT>;

  bool const& send_back = false;
  auto const& pipe_id = makePipeID(persist,send_back);
  /*
   *  Create a dispatch function so any inputs can be dispatched without any
   *  type information on the sending side
   */
  auto dispatch_fn = [pipe_id](void* input) {
    auto data = reinterpret_cast<typename SignalType::DataPtrType>(input);
    signal_holder_<SignalType>.deliverAll(pipe_id,data);
  };
  DispatchFuncType dfn = nullptr;
  if (dispatch) {
    dfn = dispatch_fn;
  }
  /*
   *  Add a new entry for pipe state to track it on this node
   */
  newPipeState(pipe_id,persist,dispatch,num_signals,num_listeners,1,dfn);
  /*
   *  Do not update the pipe state since this newly registered listener is
   *  already counted by the `1' in the newPipeState(...,1)
   */
  registerCallback<SignalType>(pipe_id,std::move(listener),false);
  return pipe_id;
}

template <typename MsgT>
PipeType PipeManagerBase::makeCallbackFunc(
  bool const& persist, FuncMsgType<MsgT> fn, bool const& dispatch,
  RefType num_signals, RefType num_listeners
) {
  using SignalType = signal::Signal<MsgT>;
  vtAssert(num_listeners > 0, "Number of listeners must be positive");
  auto const& num_refs = !persist ? num_signals / num_listeners : -1;
  auto anon = std::make_unique<callback::AnonListener<SignalType>>(
    fn, persist, num_refs
  );
  return makeCallbackAny<MsgT>(
    persist, std::move(anon), dispatch, num_signals, num_listeners
  );
}


template <typename MsgT, typename ListenerT>
void PipeManagerBase::addListenerAny(PipeType const& pipe, ListenerT&& fn) {
  using SignalType = signal::Signal<MsgT>;
  auto iter = pipe_state_.find(pipe);
  vtAssert(iter != pipe_state_.end(), "Pipe state must exist");
  auto& state = iter->second;
  if (!state.hasDispatch()) {
    auto fn2 = [pipe](void* input) {
      auto data = reinterpret_cast<typename SignalType::DataPtrType>(input);
      signal_holder_<SignalType>.deliverAll(pipe,data);
    };
    state.setDispatch(fn2);
  }
  return registerCallback<SignalType>(pipe, std::move(fn));
}

template <typename MsgT>
void PipeManagerBase::addListener(PipeType const& pipe, FuncMsgType<MsgT> fn) {
  using SignalType = signal::Signal<MsgT>;
  auto iter = pipe_state_.find(pipe);
  vtAssert(iter != pipe_state_.end(), "Pipe state must exist");
  auto const& state = iter->second;
  auto const& persist = state.isPersist();
  auto const& refs = state.refsPerListener();
  return registerCallback<SignalType>(
    pipe, std::make_unique<callback::AnonListener<SignalType>>(fn,persist,refs)
  );
}

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_BASE_IMPL_H*/
