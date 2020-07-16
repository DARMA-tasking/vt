/*
//@HEADER
// *****************************************************************************
//
//                           callback_anon_tl.impl.h
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

#if !defined INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_TL_IMPL_H
#define INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_TL_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager.fwd.h"
#include "vt/pipe/callback/callback_base_tl.h"
#include "vt/pipe/callback/anon/callback_anon_tl.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/pipe/msg/callback.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"

namespace vt { namespace pipe { namespace callback {

template <typename SerializerT>
void CallbackAnonTypeless::serialize(SerializerT& s) {}

template <typename MsgT>
void CallbackAnonTypeless::trigger(MsgT* msg, PipeType const& pipe) {
  auto const& this_node = theContext()->getNode();
  auto const& pipe_node = PipeIDBuilder::getNode(pipe);
  vt_debug_print(
    pipe, node,
    "CallbackAnonTypeless: trigger_: pipe={:x}, this_node={}\n",
    pipe, this_node
  );
  if (this_node == pipe_node) {
    triggerPipeTyped<MsgT>(pipe,msg);
  } else {
    /*
     * Set pipe type on the message envelope; use the group in the envelope in
     * indicate the pipe
     */
    setPipeType(msg->env);
    envelopeSetGroup(msg->env,pipe);
    theMsg()->sendMsg<MsgT,triggerCallbackMsgHan>(pipe_node,msg);
  }
}

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_TL_IMPL_H*/
