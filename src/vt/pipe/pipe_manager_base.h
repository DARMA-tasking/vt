/*
//@HEADER
// ************************************************************************
//
//                          pipe_manager_base.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_PIPE_PIPE_MANAGER_BASE_H
#define INCLUDED_PIPE_PIPE_MANAGER_BASE_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager.fwd.h"
#include "vt/pipe/state/pipe_state.h"
#include "vt/pipe/msg/callback.h"
#include "vt/pipe/signal/signal_holder.h"
#include "vt/pipe/callback/anon/callback_anon.fwd.h"
#include "vt/pipe/callback/handler_send/callback_send.fwd.h"
#include "vt/pipe/callback/handler_bcast/callback_bcast.fwd.h"
#include "vt/pipe/callback/proxy_send/callback_proxy_send_tl.fwd.h"
#include "vt/pipe/callback/proxy_bcast/callback_proxy_bcast_tl.fwd.h"
#include "vt/pipe/callback/anon/callback_anon_tl.fwd.h"

#include <functional>

namespace vt { namespace pipe {

struct PipeManagerBase {
  using PipeStateType    = PipeState;

  template <typename MsgT>
  using FuncMsgType      = std::function<void(MsgT*)>;
  template <typename MsgT, typename ContextT>
  using FuncMsgCtxType   = std::function<void(MsgT*, ContextT*)>;
  template <typename ContextT>
  using FuncCtxType      = std::function<void(ContextT*)>;
  using FuncType         = std::function<void(void)>;
  using FuncVoidType     = std::function<void(void)>;
  using DispatchFuncType = PipeState::DispatchFuncType;

  PipeManagerBase() = default;

  template <typename SignalT>
  friend struct pipe::callback::CallbackAnon;
  template <typename SignalT>
  friend struct pipe::callback::CallbackSend;
  template <typename SignalT>
  friend struct pipe::callback::CallbackBcast;
  friend struct pipe::callback::CallbackAnonTypeless;
  friend struct pipe::callback::CallbackProxySendTypeless;
  friend struct pipe::callback::CallbackProxyBcastTypeless;
  friend struct pipe::callback::CallbackProxySendDirect;
  friend struct pipe::callback::CallbackProxyBcastDirect;

  PipeType makeCallbackFuncVoid(
    bool const& persist, FuncType fn, bool const& dispatch = false,
    RefType num_signals = -1, RefType num_listeners = 1
  );

  template <typename MsgT>
  PipeType makeCallbackFunc(
    bool const& persist, FuncMsgType<MsgT> fn, bool const& dispatch = false,
    RefType num_signals = -1, RefType num_listeners = 1
  );

  template <typename MsgT>
  void addListener(PipeType const& pipe, FuncMsgType<MsgT> fn);
  void addListenerVoid(PipeType const& pipe, FuncType fn);

private:
  template <typename MsgT, typename ListenerT>
  PipeType makeCallbackAny(
    bool const& persist, ListenerT&& fn, bool const& dispatch = false,
    RefType num_signals = -1, RefType num_listeners = 1
  );

  template <typename MsgT, typename ListenerT>
  void addListenerAny(PipeType const& pipe, ListenerT&& fn);

public:
  static void triggerCallbackHan(CallbackMsg* msg);

  template <typename MsgT>
  static void triggerCallbackMsgHan(MsgT* msg);

protected:
  template <typename MsgT>
  void triggerPipeTyped(PipeType const& pipe, MsgT* msg);
  template <typename MsgT>
  void triggerPipeUnknown(PipeType const& pipe, MsgT* msg);
  template <typename SignalT, typename ListenerT>
  void registerCallback(
    PipeType const& pipe, ListenerT&& listener, bool update_state = true
  );
  void triggerPipe(PipeType const& pipe);
  void generalSignalTrigger(PipeType const& pipe);
  void newPipeState(
    PipeType const& pipe, bool persist, bool typeless, RefType num_sig,
    RefType num_listeners, RefType num_reg_listeners,
    DispatchFuncType fn = nullptr
  );

protected:
  PipeType makePipeID(bool const persist, bool const send_back);

private:
  template <typename SignalT>
  static signal::SignalHolder<SignalT> signal_holder_;

private:
  // the current pipe id local to this node
  PipeIDType cur_pipe_id_ = initial_pipe_id;
  // the pipe state for pipes that have a send back
  std::unordered_map<PipeType,PipeStateType> pipe_state_;
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_BASE_H*/
