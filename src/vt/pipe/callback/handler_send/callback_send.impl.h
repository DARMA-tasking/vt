/*
//@HEADER
// *****************************************************************************
//
//                             callback_send.impl.h
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

#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_IMPL_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/handler_send/callback_send.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/runnable/general.h"

namespace vt { namespace pipe { namespace callback {

template <typename MsgT>
CallbackSend<MsgT>::CallbackSend(
  HandlerType const& in_handler, NodeType const& in_send_node
) : send_node_(in_send_node), handler_(in_handler)
{ }

template <typename MsgT>
template <typename SerializerT>
void CallbackSend<MsgT>::serialize(SerializerT& s) {
  CallbackBase<SignalBaseType>::serializer(s);
  s | send_node_;
  s | handler_;
}

template <typename MsgT>
void CallbackSend<MsgT>::trigger_(SignalDataType* data, PipeType const& pid) {
  triggerDispatch<MsgT>(data,pid);
}

template <typename MsgT>
void CallbackSend<MsgT>::trigger_(SignalDataType* data) {
  vtAssert(0, "Should not be reachable in this derived class");
}

template <typename MsgT>
template <typename T>
typename CallbackSend<MsgT>::template IsVoidType<T>
CallbackSend<MsgT>::triggerDispatch(SignalDataType* data, PipeType const& pid) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    pipe, node,
    "CallbackSend: (void) trigger_: this_node={}, send_node_={}\n",
    this_node, send_node_
  );
  if (this_node == send_node_) {
    runnable::RunnableVoid::run(handler_,this_node);
  } else {
    auto msg = makeSharedMessage<CallbackMsg>(pid);
    theMsg()->sendMsg<CallbackMsg>(send_node_, handler_, msg);
  }
}

template <typename MsgT>
template <typename T>
typename CallbackSend<MsgT>::template IsNotVoidType<T>
CallbackSend<MsgT>::triggerDispatch(SignalDataType* data, PipeType const& pid) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    pipe, node,
    "CallbackSend: trigger_: this_node={}, send_node_={}\n",
    this_node, send_node_
  );
  if (this_node == send_node_) {
    auto msg = reinterpret_cast<ShortMessage*>(data);
    runnable::Runnable<ShortMessage>::run(handler_, nullptr, msg, this_node);
  } else {
    theMsg()->sendMsgAuto<SignalDataType>(send_node_, handler_, data);
  }
}


}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_IMPL_H*/
