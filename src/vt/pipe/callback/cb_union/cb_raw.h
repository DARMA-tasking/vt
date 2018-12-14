
#if !defined INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_H
#define INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_H

#include "vt/config.h"
#include "vt/pipe/callback/handler_send/callback_send_tl.h"
#include "vt/pipe/callback/handler_bcast/callback_bcast_tl.h"
#include "vt/pipe/callback/proxy_bcast/callback_proxy_bcast_tl.h"
#include "vt/pipe/callback/proxy_send/callback_proxy_send_tl.h"
#include "vt/pipe/callback/anon/callback_anon_tl.h"

#include <cstdlib>
#include <cstdint>
#include <cassert>

namespace vt { namespace pipe { namespace callback { namespace cbunion {

struct AnonCB : CallbackAnonTypeless { };

struct SendMsgCB : CallbackSendTypeless {
  SendMsgCB() = default;
  SendMsgCB(
    HandlerType const& in_handler, NodeType const& in_send_node
  ) : CallbackSendTypeless(in_handler, in_send_node)
  { }
};

struct BcastMsgCB : CallbackBcastTypeless {
  BcastMsgCB() = default;
  BcastMsgCB(
    HandlerType const& in_handler, bool const& in_include
  ) : CallbackBcastTypeless(in_handler, in_include)
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
    HandlerType const& in_handler,
    CallbackProxyBcastDirect::AutoHandlerType const& in_vrt_handler,
    bool const& in_member, VirtualProxyType const& in_proxy
  ) : CallbackProxyBcastDirect(in_handler, in_vrt_handler, in_member, in_proxy)
  { }
};

struct SendColDirCB : CallbackProxyBcastDirect {
  SendColDirCB() = default;
};

union CallbackUnion {

  CallbackUnion() : anon_cb_(AnonCB{}) { }
  CallbackUnion(CallbackUnion const&) = default;
  CallbackUnion(CallbackUnion&&) = default;
  CallbackUnion& operator=(CallbackUnion const&) = default;

  explicit CallbackUnion(SendMsgCB const& in)     : send_msg_cb_(in)      { }
  explicit CallbackUnion(BcastMsgCB const& in)    : bcast_msg_cb_(in)     { }
  explicit CallbackUnion(SendColMsgCB const& in)  : send_col_msg_cb_(in)  { }
  explicit CallbackUnion(BcastColMsgCB const& in) : bcast_col_msg_cb_(in) { }
  explicit CallbackUnion(BcastColDirCB const& in) : bcast_col_dir_cb_(in) { }
  explicit CallbackUnion(SendColDirCB const& in)  : send_col_dir_cb_(in)  { }
  explicit CallbackUnion(AnonCB const& in)        : anon_cb_(in)          { }

  AnonCB        anon_cb_;
  SendMsgCB     send_msg_cb_;
  BcastMsgCB    bcast_msg_cb_;
  SendColMsgCB  send_col_msg_cb_;
  BcastColMsgCB bcast_col_msg_cb_;
  BcastColDirCB bcast_col_dir_cb_;
  SendColDirCB  send_col_dir_cb_;
};

enum struct CallbackEnum : int8_t {
  NoCB          = 0,
  SendMsgCB     = 1,
  BcastMsgCB    = 2,
  SendColMsgCB  = 3,
  BcastColMsgCB = 4,
  BcastColDirCB = 5,
  SendColDirCB  = 6,
  AnonCB        = 7
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
    case CallbackEnum::NoCB:
      vtAssert(0, "Trying to serialize union in invalid state");
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
