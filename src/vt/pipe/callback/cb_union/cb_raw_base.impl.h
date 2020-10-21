/*
//@HEADER
// *****************************************************************************
//
//                              cb_raw_base.impl.h
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

#if !defined INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_BASE_IMPL_H
#define INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_BASE_IMPL_H

#include "vt/config.h"
#include "vt/pipe/callback/cb_union/cb_raw.h"
#include "vt/pipe/callback/cb_union/cb_raw_base.h"

#include <cassert>

namespace vt { namespace pipe { namespace callback { namespace cbunion {

template <typename MsgT>
CallbackRawBaseSingle::CallbackRawBaseSingle(CallbackTyped<MsgT> in)
  : pipe_(in.pipe_), cb_(in.cb_)
{ }

template <typename MsgT>
bool CallbackRawBaseSingle::operator==(CallbackTyped<MsgT> const& other) const {
  return equal(other);
}

template <typename MsgT, typename... Args>
void CallbackRawBaseSingle::send(Args... args) {
  sendMsg<MsgT>(makeMessage<MsgT>(std::forward<Args>(args)...));
}

template <typename MsgT>
void CallbackRawBaseSingle::send(MsgT* msg) {
  switch (cb_.active_) {
  case CallbackEnum::SendMsgCB:
    cb_.u_.send_msg_cb_.trigger<MsgT>(msg,pipe_);
    break;
  case CallbackEnum::BcastMsgCB:
    cb_.u_.bcast_msg_cb_.trigger<MsgT>(msg,pipe_);
    break;
  case CallbackEnum::AnonCB:
    cb_.u_.anon_cb_.trigger<MsgT>(msg,pipe_);
    break;
  case CallbackEnum::SendColMsgCB:
    cb_.u_.send_col_msg_cb_.trigger<MsgT>(msg,pipe_);
    break;
  case CallbackEnum::BcastColMsgCB:
    cb_.u_.bcast_col_msg_cb_.trigger<MsgT>(msg,pipe_);
    break;
  case CallbackEnum::SendColDirCB:
    cb_.u_.send_col_dir_cb_.trigger<MsgT>(msg,pipe_);
    break;
  case CallbackEnum::BcastColDirCB:
    cb_.u_.bcast_col_dir_cb_.trigger<MsgT>(msg,pipe_);
    break;
  case CallbackEnum::SendObjGrpCB:
    cb_.u_.send_obj_cb_.trigger<MsgT>(msg,pipe_);
    break;
  case CallbackEnum::BcastObjGrpCB:
    cb_.u_.bcast_obj_cb_.trigger<MsgT>(msg,pipe_);
    break;
  default:
    vtAssert(0, "Should not be reachable");
  }
}

template <typename MsgT>
void CallbackRawBaseSingle::sendMsg(messaging::MsgPtrThief<MsgT> msg) {
  send(msg.msg_.get());
}

template <typename SerializerT>
void CallbackRawBaseSingle::serialize(SerializerT& s) {
  s | cb_ | pipe_;
}


}}}} /* end namespace vt::pipe::callback::cbunion */

#endif /*INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_BASE_IMPL_H*/
