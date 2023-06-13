/*
//@HEADER
// *****************************************************************************
//
//                           callback_send_tl.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_TL_IMPL_H
#define INCLUDED_VT_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_TL_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/callback_base_tl.h"
#include "vt/pipe/callback/handler_send/callback_send_tl.h"
#include "vt/activefn/activefn.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/runnable/make_runnable.h"

namespace vt { namespace pipe { namespace callback {

template <typename SerializerT>
void CallbackSendTypeless::serialize(SerializerT& s) {
  s | send_node_;
  s | handler_;
}

template <typename MsgT>
void CallbackSendTypeless::trigger(MsgT* msg, PipeType const& pipe) {
  static_assert(not std::is_same<MsgT, NoMsg>::value, "Must not be no msg");

  auto const& this_node = theContext()->getNode();
  vt_debug_print(
    terse, pipe,
    "CallbackSendTypeless: trigger_: pipe={:x}, this_node={}, send_node_={}\n",
    pipe, this_node, send_node_
  );
  auto pmsg = promoteMsg(msg);
  if (this_node == send_node_) {
    runnable::makeRunnable(pmsg, true, handler_, this_node)
      .withTDEpochFromMsg()
      .enqueue();
  } else {
    theMsg()->sendMsg<MsgT>(send_node_, handler_, pmsg);
  }
}

}}} /* end namespace vt::pipe::callback */

#include "vt/pipe/callback/proxy_send/callback_proxy_send_tl.impl.h"

#endif /*INCLUDED_VT_PIPE_CALLBACK_HANDLER_SEND_CALLBACK_SEND_TL_IMPL_H*/
