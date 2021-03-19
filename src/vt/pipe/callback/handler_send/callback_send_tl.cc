/*
//@HEADER
// *****************************************************************************
//
//                             callback_send_tl.cc
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

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/callback_base_tl.h"
#include "vt/pipe/callback/handler_send/callback_send_tl.h"
#include "vt/pipe/msg/callback.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/context/runnable_context/td.h"
#include "vt/context/runnable_context/from_node.h"
#include "vt/context/runnable_context/set_context.h"

namespace vt { namespace pipe { namespace callback {

CallbackSendTypeless::CallbackSendTypeless(
  HandlerType const in_handler, NodeType const& in_send_node
) : send_node_(in_send_node), handler_(in_handler)
{ }

void CallbackSendTypeless::triggerVoid(PipeType const& pipe) {
  auto const& this_node = theContext()->getNode();
  vt_debug_print(
    pipe, node,
    "CallbackSendTypeless: (void) trigger_: pipe={:x}, this_node={}, "
    "send_node_={}\n",
    pipe, this_node, send_node_
  );
  if (this_node == send_node_) {
    auto r = std::make_unique<runnable::RunnableNew>(true);
    r->template addContext<ctx::TD>(theMsg()->getEpoch());
    r->template addContext<ctx::FromNode>(this_node);
    r->template addContext<ctx::SetContext>(r.get());
    r->setupHandler(RunnableEnum::Void, handler_, this_node);
    r->run();
  } else {
    auto msg = makeMessage<CallbackMsg>(pipe);
    theMsg()->sendMsg<CallbackMsg>(send_node_, handler_, msg);
  }
}

}}} /* end namespace vt::pipe::callback */
