
#if !defined INCLUDED_PIPE_PIPE_MANAGER_TL_IMPL_H
#define INCLUDED_PIPE_PIPE_MANAGER_TL_IMPL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/pipe_manager_tl.h"
#include "pipe/pipe_manager_base.h"
#include "pipe/callback/cb_union/cb_raw_base.h"
#include "pipe/callback/callback_base.h"
#include "pipe/callback/handler_send/callback_send.h"
#include "pipe/callback/handler_bcast/callback_bcast.h"
#include "pipe/callback/proxy_send/callback_proxy_send.h"
#include "pipe/callback/proxy_bcast/callback_proxy_bcast.h"
#include "pipe/callback/proxy_send/callback_proxy_send_tl.h"
#include "pipe/callback/proxy_bcast/callback_proxy_bcast_tl.h"
#include "activefn/activefn.h"
#include "context/context.h"
#include "utils/static_checks/functor.h"

#include <memory>

namespace vt { namespace pipe {

template <typename unused_>
PipeManagerTL::CallbackType PipeManagerTL::makeCallback() {
  bool const persist = true;
  bool const send_back = false;
  bool const dispatch = true;
  auto const& pipe_id = makePipeID(persist,send_back);
  newPipeState(pipe_id,persist,dispatch,-1,-1,0);
  auto cb = CallbackType(callback::cbunion::RawAnonTag,pipe_id);
  return cb;
}

template <typename T>
PipeManagerTL::CallbackMsgType<T> PipeManagerTL::makeCallbackTyped() {
  return makeCallback();
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
void PipeManagerTL::addListener(CallbackType const& cb, NodeType const& node) {
  auto const& han = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  addListenerAny<MsgT>(
    cb.getPipe(), std::make_unique<callback::CallbackSend<MsgT>>(han,node)
  );
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
void PipeManagerTL::addListenerBcast(CallbackType const& cb, bool const& inc) {
  auto const& han = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  addListenerAny<MsgT>(
    cb.getPipe(), std::make_unique<callback::CallbackBcast<MsgT>>(han,inc)
  );
}

template <typename FunctorT>
void PipeManagerTL::addListenerFunctor(
  CallbackType const& cb, NodeType const& node
) {
  using MsgT = typename util::FunctorExtractor<FunctorT>::MessageType;
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& han = auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  addListenerAny<MsgT>(
    cb.getPipe(), std::make_unique<callback::CallbackSend<MsgT>>(han,node)
  );
}

template <typename FunctorT>
void PipeManagerTL::addListenerFunctorVoid(
  CallbackType const& cb, NodeType const& node
) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& han = auto_registry::makeAutoHandlerFunctor<FunctorT,true>();
  addListenerAny<signal::SigVoidType>(
    cb.getPipe(), std::make_unique<callback::CallbackSend<signal::SigVoidType>>(
      han,node
    )
  );
}

template <typename FunctorT>
void PipeManagerTL::addListenerFunctorBcast(
  CallbackType const& cb, bool const& inc
) {
  using MsgT = typename util::FunctorExtractor<FunctorT>::MessageType;
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& han = auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  addListenerAny<MsgT>(
    cb.getPipe(), std::make_unique<callback::CallbackBcast<MsgT>>(han,inc)
  );
}

template <typename ColT, typename MsgT, PipeManagerTL::ColHanType<ColT,MsgT>* f>
PipeManagerTL::CallbackType
PipeManagerTL::makeCallbackSingleProxySend(typename ColT::ProxyType proxy) {
  bool const persist = true;
  bool const send_back = false;
  bool const dispatch = true;
  auto const& pipe_id = makePipeID(persist,send_back);
  newPipeState(pipe_id,persist,dispatch,-1,-1,0);
  auto cb = CallbackType(callback::cbunion::RawSendColMsgTag,pipe_id);
  auto const& handler = auto_registry::makeAutoHandlerCollection<ColT,MsgT,f>(
    nullptr
  );
  addListenerAny<MsgT>(
    cb.getPipe(),
    std::make_unique<callback::CallbackProxySend<ColT,MsgT>>(handler,proxy)
  );
  return cb;
}

// Single active message collection proxy bcast
template <typename ColT, typename MsgT, PipeManagerTL::ColHanType<ColT,MsgT>* f>
PipeManagerTL::CallbackType
PipeManagerTL::makeCallbackSingleProxyBcast(ColProxyType<ColT> proxy) {
  bool const persist = true;
  bool const send_back = false;
  bool const dispatch = true;
  auto const& pipe_id = makePipeID(persist,send_back);
  newPipeState(pipe_id,persist,dispatch,-1,-1,0);
  auto cb = CallbackType(callback::cbunion::RawBcastColMsgTag,pipe_id);
  auto const& handler = auto_registry::makeAutoHandlerCollection<ColT,MsgT,f>(
    nullptr
  );
  addListenerAny<MsgT>(
    cb.getPipe(),
    std::make_unique<callback::CallbackProxyBcast<ColT,MsgT>>(handler,proxy)
  );
  return cb;
}

template <typename ColT, typename MsgT, PipeManagerTL::ColHanType<ColT,MsgT>* f>
PipeManagerTL::CallbackType
PipeManagerTL::makeCallbackSingleProxyBcastDirect(ColProxyType<ColT> proxy) {
  bool const persist = true;
  bool const send_back = false;
  bool const dispatch = true;
  auto const& pipe_id = makePipeID(persist,send_back);
  newPipeState(pipe_id,persist,dispatch,-1,-1,0);
  auto const& handler = auto_registry::makeAutoHandlerCollection<ColT,MsgT,f>(
    nullptr
  );
  auto const& vrt_handler = theCollection()->getDispatchHandler<MsgT,ColT>();
  bool const member = false;
  auto cb = CallbackType(
    callback::cbunion::RawBcastColDirTag,pipe_id,handler,vrt_handler,member,
    proxy.getProxy()
  );
  addListenerAny<MsgT>(
    cb.getPipe(),
    std::make_unique<callback::CallbackProxyBcast<ColT,MsgT>>(handler,proxy)
  );
  return cb;
}

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

template <typename FunctorT>
PipeManagerTL::CallbackType
PipeManagerTL::makeCallbackFunctorSend(NodeType const& send_to_node) {
  using MsgT = typename util::FunctorExtractor<FunctorT>::MessageType;
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler =
    auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  auto cb = CallbackType(
    callback::cbunion::RawSendMsgTag,new_pipe_id,handler,send_to_node
  );
  return cb;
}

template <typename FunctorT>
PipeManagerTL::CallbackType
PipeManagerTL::makeCallbackFunctorBcast(bool const& inc) {
  using MsgT = typename util::FunctorExtractor<FunctorT>::MessageType;
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler =
    auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  auto cb = CallbackType(
    callback::cbunion::RawBcastMsgTag,new_pipe_id,handler,inc
  );
  return cb;
}

template <typename FunctorT>
PipeManagerTL::CallbackType
PipeManagerTL::makeCallbackFunctorSendVoid(NodeType const& send_to_node) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler = auto_registry::makeAutoHandlerFunctor<FunctorT,true>();
  auto cb = CallbackType(
    callback::cbunion::RawSendMsgTag,new_pipe_id,handler,send_to_node
  );
  return cb;
}

template <typename FunctorT>
PipeManagerTL::CallbackType
PipeManagerTL::makeCallbackFunctorBcastVoid(bool const& inc) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler = auto_registry::makeAutoHandlerFunctor<FunctorT,true>();
  auto cb = CallbackType(
    callback::cbunion::RawBcastMsgTag,new_pipe_id,handler,inc
  );
  return cb;
}

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_TL_IMPL_H*/
