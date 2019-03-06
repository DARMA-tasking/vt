/*
//@HEADER
// ************************************************************************
//
//                          callback_anon.impl.h
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

#if !defined INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_IMPL_H
#define INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/anon/callback_anon.h"
#include "vt/pipe/pipe_manager.fwd.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/msg/callback.h"
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
    triggerPipe(pid);
  } else {
    auto msg = makeSharedMessage<CallbackMsg>(pid);
    theMsg()->sendMsg<CallbackMsg,triggerCallbackHan>(
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
    triggerPipeTyped<T>(pid,data);
  } else {
    /*
     * Set pipe type on the message envelope; use the group in the envelope in
     * indicate the pipe
     */
    setPipeType(data->env);
    envelopeSetGroup(data->env,pid);
    theMsg()->sendMsgAuto<T,triggerCallbackMsgHan>(pipe_node, data);
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
