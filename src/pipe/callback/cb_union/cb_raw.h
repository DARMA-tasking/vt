
#if !defined INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_H
#define INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_H

#include "config.h"
#include "pipe/callback/handler_send/callback_send_tl.h"
#include "pipe/callback/handler_bcast/callback_bcast_tl.h"
#include "pipe/callback/proxy_bcast/callback_proxy_bcast_tl.h"
#include "pipe/callback/proxy_send/callback_proxy_send_tl.h"
#include "pipe/callback/anon/callback_anon_tl.h"

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

union CallbackUnion {

  CallbackUnion() : anon_cb_(AnonCB{}) { }
  CallbackUnion(CallbackUnion const&) = default;
  CallbackUnion(CallbackUnion&&) = default;
  CallbackUnion& operator=(CallbackUnion const&) = default;

  explicit CallbackUnion(SendMsgCB const& in)     : send_msg_cb_(in)      { }
  explicit CallbackUnion(BcastMsgCB const& in)    : bcast_msg_cb_(in)     { }
  explicit CallbackUnion(SendColMsgCB const& in)  : send_col_msg_cb_(in)  { }
  explicit CallbackUnion(BcastColMsgCB const& in) : bcast_col_msg_cb_(in) { }
  explicit CallbackUnion(AnonCB const& in)        : anon_cb_(in)          { }

  AnonCB        anon_cb_;
  SendMsgCB     send_msg_cb_;
  BcastMsgCB    bcast_msg_cb_;
  SendColMsgCB  send_col_msg_cb_;
  BcastColMsgCB bcast_col_msg_cb_;
};

enum struct CallbackEnum : int8_t {
  NoCB          = 0,
  SendMsgCB     = 1,
  BcastMsgCB    = 2,
  SendColMsgCB  = 3,
  BcastColMsgCB = 4,
  AnonCB        = 5
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

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | u_ | active_;
  }

  CallbackUnion u_;
  CallbackEnum active_ = CallbackEnum::NoCB;
};

}}}} /* end namespace vt::pipe::callback::cbunion */

#endif /*INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_H*/
