
#if !defined INCLUDED_PIPE_PIPE_MANAGER_TL_IMPL_H
#define INCLUDED_PIPE_PIPE_MANAGER_TL_IMPL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/pipe_manager_tl.h"
#include "pipe/pipe_manager_base.h"
#include "pipe/callback/cb_union/cb_raw_base.h"
#include "activefn/activefn.h"
#include "context/context.h"

namespace vt { namespace pipe {

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
PipeManagerTL::CallbackType
PipeManagerTL::makeCallbackSingleSend(NodeType const& send_to_node) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  auto cb = CallbackType(
    callback::cbunion::RawSendMsgTag,new_pipe_id,handler,send_to_node
  );
  return cb;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
PipeManagerTL::CallbackType
PipeManagerTL::makeCallbackSingleBcast(bool const& inc) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  auto cb = CallbackType(
    callback::cbunion::RawBcastMsgTag,new_pipe_id,handler,inc
  );
  return cb;
}

template <typename unused_>
PipeManagerTL::CallbackType
PipeManagerTL:: makeCallbackSingleAnonVoid(FuncVoidType fn) {
  auto const& new_pipe_id = makeCallbackFuncVoid(true,fn,true);
  auto cb = CallbackType(callback::cbunion::RawAnonTag,new_pipe_id);

  debug_print(
    pipe, node,
    "makeCallbackSingleAnonVoid: persist={}, pipe={:x}\n",
    true, new_pipe_id
  );

  return cb;
}

template <typename MsgT>
PipeManagerTL::CallbackType
PipeManagerTL:: makeCallbackSingleAnon(FuncMsgType<MsgT> fn) {
  auto const& new_pipe_id = makeCallbackFunc<MsgT>(true,fn,true);
  auto cb = CallbackType(callback::cbunion::RawAnonTag,new_pipe_id);

  debug_print(
    pipe, node,
    "makeCallbackSingleAnon: persist={}, pipe={:x}\n",
    true, new_pipe_id
  );

  return cb;
}


}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_TL_IMPL_H*/
