/*
//@HEADER
// *****************************************************************************
//
//                                   cb_raw.h
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

#if !defined INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_H
#define INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_H

#include "vt/config.h"
#include "vt/pipe/callback/handler_send/callback_send_tl.h"
#include "vt/pipe/callback/handler_bcast/callback_bcast_tl.h"
#include "vt/pipe/callback/proxy_bcast/callback_proxy_bcast_tl.h"
#include "vt/pipe/callback/proxy_send/callback_proxy_send_tl.h"
#include "vt/pipe/callback/objgroup_bcast/callback_objgroup_bcast.h"
#include "vt/pipe/callback/objgroup_send/callback_objgroup_send.h"
#include "vt/pipe/callback/anon/callback_anon_tl.h"

#include <cstdlib>
#include <cstdint>
#include <cassert>

namespace vt { namespace pipe { namespace callback { namespace cbunion {

struct AnonCB : CallbackAnonTypeless { };

struct SendMsgCB : CallbackSendTypeless {
  SendMsgCB() = default;
  SendMsgCB(
    HandlerType const in_handler, NodeType const& in_send_node
  ) : CallbackSendTypeless(in_handler, in_send_node)
  { }
};

struct BcastMsgCB : CallbackBcastTypeless {
  BcastMsgCB() = default;
  BcastMsgCB(
    HandlerType const in_handler
  ) : CallbackBcastTypeless(in_handler)
  { }
};

struct SendColMsgCB : CallbackProxySendTypeless {
  SendColMsgCB() = default;
};

struct BcastColMsgCB : CallbackProxyBcastTypeless {
  BcastColMsgCB() = default;
};

struct BcastColDirCB : CallbackProxyBcastDirect {
  BcastColDirCB() = default;
  BcastColDirCB(
    HandlerType const in_handler,
    CallbackProxyBcastDirect::AutoHandlerType const in_vrt_handler,
    VirtualProxyType const& in_proxy
  ) : CallbackProxyBcastDirect(in_handler, in_vrt_handler, in_proxy)
  { }
};

struct SendColDirCB : CallbackProxyBcastDirect {
  SendColDirCB() = default;
};

struct BcastObjGrpCB : CallbackObjGroupBcast {
  BcastObjGrpCB() = default;
  BcastObjGrpCB(HandlerType in_handler, ObjGroupProxyType in_proxy)
    : CallbackObjGroupBcast(in_handler, in_proxy)
  { }
};

struct SendObjGrpCB : CallbackObjGroupSend {
  SendObjGrpCB() = default;
  SendObjGrpCB(HandlerType in_handler, ObjGroupProxyType in_proxy, NodeType in_n)
    : CallbackObjGroupSend(in_handler, in_proxy, in_n)
  { }
};

union CallbackUnion {

  CallbackUnion() : anon_cb_(AnonCB{}) { }
  CallbackUnion(CallbackUnion const&) = default;
  CallbackUnion(CallbackUnion&&) = default;
  CallbackUnion& operator=(CallbackUnion const&) = default;

  explicit CallbackUnion(SendMsgCB const& in)       : send_msg_cb_(in)      { }
  explicit CallbackUnion(BcastMsgCB const& in)      : bcast_msg_cb_(in)     { }
  explicit CallbackUnion(SendColMsgCB const& in)    : send_col_msg_cb_(in)  { }
  explicit CallbackUnion(BcastColMsgCB const& in)   : bcast_col_msg_cb_(in) { }
  explicit CallbackUnion(BcastColDirCB const& in)   : bcast_col_dir_cb_(in) { }
  explicit CallbackUnion(SendColDirCB const& in)    : send_col_dir_cb_(in)  { }
  explicit CallbackUnion(AnonCB const& in)          : anon_cb_(in)          { }
  explicit CallbackUnion(BcastObjGrpCB const& in)   : bcast_obj_cb_(in)     { }
  explicit CallbackUnion(SendObjGrpCB const& in)    : send_obj_cb_(in)      { }

  AnonCB        anon_cb_;
  SendMsgCB     send_msg_cb_;
  BcastMsgCB    bcast_msg_cb_;
  SendColMsgCB  send_col_msg_cb_;
  BcastColMsgCB bcast_col_msg_cb_;
  BcastColDirCB bcast_col_dir_cb_;
  SendColDirCB  send_col_dir_cb_;
  BcastObjGrpCB bcast_obj_cb_;
  SendObjGrpCB  send_obj_cb_;
};

enum struct CallbackEnum : int8_t {
  NoCB          = 0,
  SendMsgCB     = 1,
  BcastMsgCB    = 2,
  SendColMsgCB  = 3,
  BcastColMsgCB = 4,
  BcastColDirCB = 5,
  SendColDirCB  = 6,
  AnonCB        = 7,
  BcastObjGrpCB = 8,
  SendObjGrpCB  = 9
};

struct GeneralCallback {
  GeneralCallback() = default;
  GeneralCallback(GeneralCallback const&) = default;
  GeneralCallback(GeneralCallback&&) = default;
  GeneralCallback& operator=(GeneralCallback const&) = default;

  explicit GeneralCallback(SendMsgCB const& in)
    : u_(in), active_(CallbackEnum::SendMsgCB)
  { }
  explicit GeneralCallback(BcastMsgCB const& in)
    : u_(in), active_(CallbackEnum::BcastMsgCB)
  { }
  explicit GeneralCallback(SendColMsgCB const& in)
    : u_(in), active_(CallbackEnum::SendColMsgCB)
  { }
  explicit GeneralCallback(BcastColMsgCB const& in)
    : u_(in), active_(CallbackEnum::BcastColMsgCB)
  { }
  explicit GeneralCallback(AnonCB const& in)
    : u_(in), active_(CallbackEnum::AnonCB)
  { }
  explicit GeneralCallback(BcastColDirCB const& in)
    : u_(in), active_(CallbackEnum::BcastColDirCB)
  { }
  explicit GeneralCallback(SendColDirCB const& in)
    : u_(in), active_(CallbackEnum::SendColDirCB)
  { }
  explicit GeneralCallback(BcastObjGrpCB const& in)
    : u_(in), active_(CallbackEnum::BcastObjGrpCB)
  { }
  explicit GeneralCallback(SendObjGrpCB const& in)
    : u_(in), active_(CallbackEnum::SendObjGrpCB)
  { }

  bool operator==(GeneralCallback const& other) const {
    bool const same_active = other.active_ == active_;
    if (same_active) {
      switch (active_) {
      case CallbackEnum::AnonCB:
         return u_.anon_cb_ == other.u_.anon_cb_;
      case CallbackEnum::SendMsgCB:
        return u_.send_msg_cb_ == other.u_.send_msg_cb_;
      case CallbackEnum::SendColMsgCB:
        return u_.send_col_msg_cb_ == other.u_.send_col_msg_cb_;
      case CallbackEnum::BcastMsgCB:
        return u_.bcast_msg_cb_ == other.u_.bcast_msg_cb_;
      case CallbackEnum::BcastColMsgCB:
        return u_.bcast_col_msg_cb_ == other.u_.bcast_col_msg_cb_;
      case CallbackEnum::BcastColDirCB:
        return u_.bcast_col_dir_cb_ == other.u_.bcast_col_dir_cb_;
      case CallbackEnum::BcastObjGrpCB:
        return u_.bcast_obj_cb_ == other.u_.bcast_obj_cb_;
      case CallbackEnum::SendObjGrpCB:
        return u_.send_obj_cb_ == other.u_.send_obj_cb_;
      case CallbackEnum::NoCB: return true;
      default: return false;
      }
    } else {
      return false;
    }
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    using EnumDataType = typename std::underlying_type<CallbackEnum>::type;
    EnumDataType val = static_cast<EnumDataType>(active_);
    s | val;
    active_ = static_cast<CallbackEnum>(val);
    switch (active_) {
    case CallbackEnum::AnonCB:
      s | u_.anon_cb_;
      break;
    case CallbackEnum::SendMsgCB:
      s | u_.send_msg_cb_;
      break;
    case CallbackEnum::SendColMsgCB:
      s | u_.send_col_msg_cb_;
      break;
    case CallbackEnum::BcastMsgCB:
      s | u_.bcast_msg_cb_;
      break;
    case CallbackEnum::BcastColMsgCB:
      s | u_.bcast_col_msg_cb_;
      break;
    case CallbackEnum::BcastColDirCB:
      s | u_.bcast_col_dir_cb_;
      break;
    case CallbackEnum::BcastObjGrpCB:
      s | u_.bcast_obj_cb_;
      break;
    case CallbackEnum::SendObjGrpCB:
      s | u_.send_obj_cb_;
      break;
    case CallbackEnum::NoCB:
      // Serializing empty callback!
      break;
    default:
      vtAssert(0, "Should be unreachable");
      break;
    }
  }

  bool null()  const { return active_ == CallbackEnum::NoCB; }
  bool valid() const { return !null(); }

  CallbackUnion u_;
  CallbackEnum active_ = CallbackEnum::NoCB;
};

}}}} /* end namespace vt::pipe::callback::cbunion */

#endif /*INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_H*/
