/*
//@HEADER
// *****************************************************************************
//
//                             callback_bcast_tl.cc
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
#include "vt/pipe/callback/handler_bcast/callback_bcast_tl.h"
#include "vt/pipe/msg/callback.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/runnable/general.h"

#include <cassert>

namespace vt { namespace pipe { namespace callback {

CallbackBcastTypeless::CallbackBcastTypeless(
  HandlerType const& in_handler, bool const& in_include
) : handler_(in_handler), include_sender_(in_include)
{ }

void CallbackBcastTypeless::triggerVoid(PipeType const& pipe) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    pipe, node,
    "CallbackBcast: (void) trigger_: pipe={:x}, this_node={}, "
    "include_sender_={}\n",
    pipe, this_node, include_sender_
  );
  auto msg = makeSharedMessage<CallbackMsg>(pipe);
  theMsg()->broadcastMsg<CallbackMsg>(handler_,msg);
  if (include_sender_) {
    runnable::RunnableVoid::run(handler_,this_node);
  }
}

}}} /* end namespace vt::pipe::callback */
