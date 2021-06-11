/*
//@HEADER
// *****************************************************************************
//
//                             pipe_manager_base.cc
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

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager_base.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/pipe/state/pipe_state.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/context/context.h"
#include "vt/pipe/signal/signal_holder.h"
#include "vt/pipe/callback/anon/callback_anon_listener.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/pipe/msg/callback.h"
#include "vt/context/context.h"

#include <cassert>
#include <cstdlib>
#include <unordered_map>
#include <memory>
#include <functional>

namespace vt { namespace pipe {

/*virtual*/ PipeManagerBase::~PipeManagerBase() {
  // Run all the cleanup lambdas to clear signal holders
  for (auto&& elm : signal_cleanup_fns_) {
    elm.second();
  }
}

void PipeManagerBase::newPipeState(
  PipeType const& pipe, bool persist, bool typeless, RefType num_signals,
  RefType num_listeners, RefType num_reg_listeners, DispatchFuncType fn
) {
  /*
   *  Create pipe state of `pipe' to track the state locally on this node
   */
  auto iter = pipe_state_.find(pipe);
  vtAssert(iter == pipe_state_.end(), "State must not exist for new pipe");
  vtAssert((persist || num_signals > 0), "Valid signal count non-persist");
  auto state_in =
    !persist ?
    PipeState{pipe,num_signals,num_listeners,typeless} :
    PipeState(pipe,typeless);
  pipe_state_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(pipe),
    std::forward_as_tuple(std::move(state_in))
  );
  iter = pipe_state_.find(pipe);
  vtAssert(iter != pipe_state_.end(), "State must exist for new pipe");
  auto& state = iter->second;
  /*
   *  Increment (manually) the number of registered listeners
   */
  for (int i = 0; i < num_reg_listeners; i++) {
    state.listenerReg();
  }
  if (fn) {
    state.setDispatch(fn);
  }
}

PipeType PipeManagerBase::makeCallbackFuncVoid(
  bool const& persist, FuncType fn, bool const& dispatch, RefType num_signals,
  RefType num_listeners
) {
  using SignalType = signal::SignalVoid;

  bool const& send_back = false;
  auto const& pipe_id = makePipeID(persist,send_back);
  auto dispatch_fn = [pipe_id](void*) {
    signal_holder_<SignalType>.deliverAll(pipe_id,nullptr);
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
   *  Register a listener for this signal to trigger the anon function when the
   *  callback is invoked
   */
  vtAssert(num_listeners > 0, "Number of listeners must be positive");
  auto const& num_refs = !persist ? num_signals / num_listeners : -1;
  auto closure = [fn](typename SignalType::DataType*){ fn(); };
  auto anon = std::make_unique<callback::AnonListener<SignalType>>(
    closure, persist, num_refs
  );
  /*
   *  Do not update the pipe state since this newly registered listener is
   *  already counted by the `1' in the newPipeState(...,1)
   */
  registerCallback<SignalType>(pipe_id,std::move(anon),false);

  return pipe_id;
}

void PipeManagerBase::addListenerVoid(PipeType const& pipe, FuncType fn) {
  using SignalType = signal::SignalVoid;
  auto closure = [fn](typename SignalType::DataType*){ fn(); };
  return addListener<typename SignalType::DataType>(pipe, closure);
}

void PipeManagerBase::generalSignalTrigger(PipeType const& pipe) {
  auto iter = pipe_state_.find(pipe);
  vtAssert(iter != pipe_state_.end(), "Must exist");
  auto& state = iter->second;
  state.signalRecv();
  auto const& automatic = state.isAutomatic();
  if (automatic) {
    if (state.finished()) {
      // Remove from the list of active pipe states
      pipe_state_.erase(iter);
    }
  }
}

void PipeManagerBase::triggerPipe(PipeType const& pipe) {
  using SignalType = signal::SignalVoid;

  auto const& exists = signal_holder_<SignalType>.exists(pipe);

  vt_debug_print(
    normal, pipe,
    "PipeManagerBase: triggerPipe: pipe={:x}, exists={}: delivering\n",
    pipe, exists
  );

  if (exists) {
    signal_holder_<SignalType>.deliverAll(pipe, nullptr);
  } else {
    triggerPipeUnknown<void>(pipe,nullptr);
  }

  generalSignalTrigger(pipe);
}

/*static*/ void PipeManagerBase::triggerCallbackHan(CallbackMsg* msg) {
  auto const& pid = msg->getPipe();

  vt_debug_print(
    normal, pipe,
    "PipeManagerBase: triggerCallbackHan pipe={:x}\n",
    pid
  );

  theCB()->triggerPipe(pid);
}

PipeType PipeManagerBase::makePipeID(bool const persist, bool const send_back) {
  auto const& this_node = theContext()->getNode();
  auto const next_id = cur_pipe_id_++;
  auto const pipe_id = PipeIDBuilder::createPipeID(
    next_id,this_node,send_back,persist
  );
  return pipe_id;
}

}} /* end namespace vt::pipe */
