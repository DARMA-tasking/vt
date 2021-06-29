/*
//@HEADER
// *****************************************************************************
//
//                                cb_raw_base.cc
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


#include "vt/config.h"
#include "vt/pipe/callback/cb_union/cb_raw_base.h"

namespace vt { namespace pipe { namespace callback { namespace cbunion {

CallbackRawBaseSingle::CallbackRawBaseSingle(
  RawSendMsgTagType, PipeType const& in_pipe, HandlerType const in_handler,
  NodeType const& in_node
) : pipe_(in_pipe), cb_(SendMsgCB{in_handler,in_node})
{ }

CallbackRawBaseSingle::CallbackRawBaseSingle(
  RawBcastMsgTagType, PipeType const& in_pipe, HandlerType const in_handler
) : pipe_(in_pipe), cb_(BcastMsgCB{in_handler})
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
  RawBcastColDirTagType, PipeType const& in_pipe, HandlerType const in_handler,
  AutoHandlerType const in_vrt, VirtualProxyType const& in_proxy
) : pipe_(in_pipe), cb_(BcastColDirCB{in_handler, in_vrt, in_proxy})
{ }

CallbackRawBaseSingle::CallbackRawBaseSingle(
  RawBcastObjGrpTagType, PipeType in_pipe, HandlerType in_handler,
  ObjGroupProxyType in_proxy
) : pipe_(in_pipe),  cb_(BcastObjGrpCB{in_handler,in_proxy})
{ }

CallbackRawBaseSingle::CallbackRawBaseSingle(
  RawSendObjGrpTagType, PipeType in_pipe, HandlerType in_handler,
  ObjGroupProxyType in_proxy, NodeType in_node
) : pipe_(in_pipe),  cb_(SendObjGrpCB{in_handler,in_proxy,in_node})
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
  case CallbackEnum::BcastObjGrpCB:
    vtAssert(0, "void dispatch not allowed for bcast collection msg callback");
    break;
  case CallbackEnum::SendObjGrpCB:
    vtAssert(0, "void dispatch not allowed for send collection msg callback");
    break;
  default:
    vtAssert(0, "Should not be reachable");
  }
}

}}}} /* end namespace vt::pipe::callback::cbunion */
