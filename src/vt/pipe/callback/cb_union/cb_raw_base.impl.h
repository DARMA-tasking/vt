
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
  default:
    vtAssert(0, "Should not be reachable");
  }
}

template <typename SerializerT>
void CallbackRawBaseSingle::serialize(SerializerT& s) {
  s | cb_ | pipe_;
}


}}}} /* end namespace vt::pipe::callback::cbunion */

#endif /*INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_BASE_IMPL_H*/
