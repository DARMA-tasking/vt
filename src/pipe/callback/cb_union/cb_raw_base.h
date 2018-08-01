
#if !defined INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_BASE_H
#define INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_BASE_H

#include "config.h"
#include "pipe/callback/cb_union/cb_raw.h"

#include <cassert>

namespace vt { namespace pipe { namespace callback { namespace cbunion {

static struct RawAnonTagType        { } RawAnonTag        { };
static struct RawSendMsgTagType     { } RawSendMsgTag     { };
static struct RawBcastMsgTagType    { } RawBcastMsgTag    { };
static struct RawSendColMsgTagType  { } RawSendColMsgTag  { };
static struct RawBcastColMsgTagType { } RawBcastColMsgTag { };

struct CallbackRawBaseSingle {

  CallbackRawBaseSingle() = default;
  CallbackRawBaseSingle(
    RawSendMsgTagType, PipeType const& in_pipe, HandlerType const& in_handler,
    NodeType const& in_node
  );
  CallbackRawBaseSingle(
    RawBcastMsgTagType, PipeType const& in_pipe, HandlerType const& in_handler,
    bool const& in_inc
  );

  template <typename MsgT>
  void send(MsgT* msg);

  template <typename SerializerT>
  void serialize(SerializerT& s);

private:
  PipeType pipe_ = no_pipe;
  GeneralCallback cb_;
};

struct CallbackRawBase {

};

}}}} /* end namespace vt::pipe::callback::cbunion */

#include "pipe/callback/cb_union/cb_raw_base.impl.h"

#endif /*INCLUDED_PIPE_CALLBACK_CB_UNION_CB_RAW_BASE_H*/
