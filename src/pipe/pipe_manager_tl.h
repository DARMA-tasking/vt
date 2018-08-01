
#if !defined INCLUDED_PIPE_PIPE_MANAGER_TL_H
#define INCLUDED_PIPE_PIPE_MANAGER_TL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/pipe_manager_base.h"
#include "pipe/callback/cb_union/cb_raw_base.h"
#include "activefn/activefn.h"

namespace vt { namespace pipe {

struct PipeManagerTL : virtual PipeManagerBase {
  using CallbackType = callback::cbunion::CallbackRawBaseSingle;

  /*
   *  Untyped variants of callbacks: uses union to dispatch
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  CallbackType makeCallbackSingleSend(NodeType const& send_to_node);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  CallbackType makeCallbackSingleBcast(bool const& inc);

  template <typename=void>
  CallbackType makeCallbackSingleAnonVoid(FuncVoidType fn);

  template <typename MsgT>
  CallbackType makeCallbackSingleAnon(FuncMsgType<MsgT> fn);
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_TL_H*/
