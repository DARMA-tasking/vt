/*
//@HEADER
// ************************************************************************
//
//                          cb_raw_base.cc
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


#include "vt/config.h"
#include "vt/pipe/callback/cb_union/cb_raw_base.h"

namespace vt { namespace pipe { namespace callback { namespace cbunion {

CallbackRawBaseSingle::CallbackRawBaseSingle(
  RawSendMsgTagType, PipeType const& in_pipe, HandlerType const& in_handler,
  NodeType const& in_node
) : pipe_(in_pipe), cb_(SendMsgCB{in_handler,in_node})
{ }

CallbackRawBaseSingle::CallbackRawBaseSingle(
  RawBcastMsgTagType, PipeType const& in_pipe, HandlerType const& in_handler,
  bool const& in_inc
) : pipe_(in_pipe), cb_(BcastMsgCB{in_handler,in_inc})
{ }

CallbackRawBaseSingle::CallbackRawBaseSingle(
  RawAnonTagType, PipeType const& in_pipe
) : pipe_(in_pipe), cb_(AnonCB{})
{ }

CallbackRawBaseSingle::CallbackRawBaseSingle(
  RawSendColMsgTagType, PipeType const& in_pipe
) : pipe_(in_pipe), cb_(SendColMsgCB{})
{ }

CallbackRawBaseSingle::CallbackRawBaseSingle(
  RawBcastColMsgTagType, PipeType const& in_pipe
) : pipe_(in_pipe), cb_(BcastColMsgCB{})
{ }

CallbackRawBaseSingle::CallbackRawBaseSingle(
  RawBcastColDirTagType, PipeType const& in_pipe,
  HandlerType const& in_handler, AutoHandlerType const& in_vrt,
  bool const& in_member, VirtualProxyType const& in_proxy
) : pipe_(in_pipe), cb_(BcastColDirCB{in_handler,in_vrt,in_member,in_proxy})
 { }

// CallbackRawBaseSingle::CallbackRawBaseSingle(
//   RawSendColDirTagType, PipeType const& in_pipe,
//   HandlerType const& in_handler, AutoHandlerType const& in_vrt_handler,
//   void* in_index
// ) : pipe_(in_pipe), cb_(SendColDirCB{in_handler,in_vrt_handler,in_index})
//  { }

void CallbackRawBaseSingle::send() {
  switch (cb_.active_) {
  case CallbackEnum::SendMsgCB:
    cb_.u_.send_msg_cb_.triggerVoid(pipe_);
    break;
  case CallbackEnum::BcastMsgCB:
    cb_.u_.bcast_msg_cb_.triggerVoid(pipe_);
    break;
  case CallbackEnum::AnonCB:
    cb_.u_.anon_cb_.triggerVoid(pipe_);
    break;
  case CallbackEnum::SendColMsgCB:
    vtAssert(0, "void dispatch not allowed for send collection msg callback");
    break;
  case CallbackEnum::BcastColMsgCB:
    vtAssert(0, "void dispatch not allowed for bcast collection msg callback");
    break;
  case CallbackEnum::BcastColDirCB:
    vtAssert(0, "void dispatch not allowed for bcast collection msg callback");
    break;
  case CallbackEnum::SendColDirCB:
    vtAssert(0, "void dispatch not allowed for send collection msg callback");
    break;
  default:
    vtAssert(0, "Should not be reachable");
  }
}

}}}} /* end namespace vt::pipe::callback::cbunion */
