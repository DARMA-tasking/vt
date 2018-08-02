
#if !defined INCLUDED_PIPE_PIPE_MANAGER_TYPED_IMPL_H
#define INCLUDED_PIPE_PIPE_MANAGER_TYPED_IMPL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/pipe_manager.h"
#include "pipe/state/pipe_state.h"
#include "pipe/interface/remote_container_msg.h"
#include "pipe/interface/send_container.h"
#include "pipe/interface/callback_direct.h"
#include "pipe/pipe_manager_construct.h"
#include "pipe/id/pipe_id.h"
#include "context/context.h"
#include "registry/auto/auto_registry.h"

#include <cassert>
#include <tuple>
#include <type_traits>
#include <array>

namespace vt { namespace pipe {

template <typename MsgT>
PipeManagerTyped::CallbackAnonType<MsgT>
PipeManagerTyped::makeCallbackSingleAnonTyped(
  bool const persist, FuncMsgType<MsgT> fn
) {
  auto const& new_pipe_id = makeCallbackFunc<MsgT>(persist,fn);
  auto container = CallbackAnonType<MsgT>{new_pipe_id};

  debug_print(
    pipe, node,
    "makeCallbackSingleAnonTyped: persist={}, pipe={:x}\n",
    persist, new_pipe_id
  );

  return container;
}

template <typename unused_>
PipeManagerTyped::CallbackAnonVoidType
PipeManagerTyped::makeCallbackSingleAnonVoidTyped(
  bool const persist, FuncVoidType fn
) {
  auto const& new_pipe_id = makeCallbackFuncVoid(persist,fn);
  auto container = CallbackAnonVoidType{new_pipe_id};

  debug_print(
    pipe, node,
    "makeCallbackSingleAnonVoidTyped: persist={}, pipe={:x}\n",
    persist, new_pipe_id
  );

  return container;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
PipeManagerTyped::CallbackSendType<MsgT>
PipeManagerTyped::makeCallbackSingleSendTyped(
  bool const is_persist, NodeType const& send_to_node
) {
  auto const& new_pipe_id = makePipeID(is_persist,false);
  auto const& handler = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  auto container = CallbackSendType<MsgT>(
    new_pipe_id,handler,send_to_node
  );
  return container;
}

template <typename FunctorT, typename MsgT>
PipeManagerTyped::CallbackSendType<MsgT>
PipeManagerTyped::makeCallbackSingleSendFunctorTyped(
  bool const is_persist, NodeType const& send_to_node
) {
  auto const& new_pipe_id = makePipeID(is_persist,false);
  auto const& handler =
    auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  auto container = CallbackSendType<MsgT>(new_pipe_id,handler,send_to_node);
  return container;
}

template <typename FunctorT>
PipeManagerTyped::CallbackSendVoidType
PipeManagerTyped::makeCallbackSingleSendFunctorVoidTyped(
  bool const is_persist, NodeType const& send_to_node
) {
  auto const& new_pipe_id = makePipeID(is_persist,false);
  auto const& handler = auto_registry::makeAutoHandlerFunctor<FunctorT,true>();
  auto container = CallbackSendVoidType(new_pipe_id,handler,send_to_node);
  return container;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
PipeManagerTyped::CallbackBcastType<MsgT>
PipeManagerTyped::makeCallbackSingleBcastTyped(bool const inc) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  auto container = CallbackBcastType<MsgT>(
    new_pipe_id,handler,inc
  );
  return container;
}

template <typename FunctorT, typename MsgT>
PipeManagerTyped::CallbackBcastType<MsgT>
PipeManagerTyped::makeCallbackSingleBcastFunctorTyped(bool const inc) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler =
    auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  auto container = CallbackBcastType<MsgT>(
    new_pipe_id,handler,inc
  );
  return container;
}

template <typename FunctorT>
PipeManagerTyped::CallbackBcastVoidType
PipeManagerTyped::makeCallbackSingleBcastFunctorVoidTyped(bool const inc) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& handler = auto_registry::makeAutoHandlerFunctor<FunctorT,true>();
  auto container = CallbackBcastVoidType(new_pipe_id,handler,inc);
  return container;
}

template <
  typename ColT,
  typename MsgT,
  vrt::collection::ActiveColTypedFnType<MsgT,ColT> *f
>
PipeManagerTyped::CallbackProxyBcastType<ColT,MsgT>
PipeManagerTyped::makeCallbackSingleProxyBcastTyped(
  CollectionProxy<ColT,typename ColT::IndexType> proxy
) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& han = auto_registry::makeAutoHandlerCollection<ColT,MsgT,f>(
    nullptr
  );
  auto container = CallbackProxyBcastType<ColT,MsgT>(new_pipe_id,han,proxy);
  return container;
}

template <
  typename ColT,
  typename MsgT,
  vrt::collection::ActiveColTypedFnType<MsgT,ColT> *f
>
PipeManagerTyped::CallbackProxySendType<ColT,MsgT>
PipeManagerTyped::makeCallbackSingleProxySendTyped(
  typename ColT::ProxyType proxy
) {
  auto const& new_pipe_id = makePipeID(true,false);
  auto const& han = auto_registry::makeAutoHandlerCollection<ColT,MsgT,f>(
    nullptr
  );
  auto container = CallbackProxySendType<ColT,MsgT>(new_pipe_id,han,proxy);
  return container;
}

template <typename MsgT, ActiveTypedFnType<MsgT>*... f>
interface::CallbackDirectSendMulti<
  MsgT,
  typename RepeatNImpl<sizeof...(f),callback::CallbackSend<MsgT>>::ResultType
>
PipeManagerTyped::makeCallbackMultiSendTyped(
  bool const is_persist, NodeType const& send_to_node
) {
  using CBSendT = callback::CallbackSend<MsgT>;
  using ConsT = std::tuple<NodeType>;
  using TupleConsT = typename RepeatNImpl<sizeof...(f),ConsT>::ResultType;
  using ConstructMeta = ConstructCallbacks<CBSendT,TupleConsT,MsgT,f...>;
  using TupleCBType = typename ConstructMeta::ResultType;

  auto const& new_pipe_id = makePipeID(is_persist,false);
  std::array<NodeType,sizeof...(f)> send_node_array;
  send_node_array.fill(send_to_node);
  auto const cons = TupleConsT{send_node_array};
  auto const tuple = ConstructMeta::make(cons);
  auto rcm = interface::CallbackDirectSendMulti<MsgT,TupleCBType>(
    interface::CallbackDirectSendMultiTag,new_pipe_id,tuple
  );
  return rcm;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
auto PipeManagerTyped::pushTargetBcast(CallbackT in, bool const& inc) {
  auto const& han = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return std::tuple_cat(
    std::make_tuple(callback::CallbackBcast<MsgT>(han,inc)), in
  );
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
auto PipeManagerTyped::pushTargetBcast(bool const& inc) {
  auto const& han = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return std::make_tuple(callback::CallbackBcast<MsgT>(han,inc));
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
auto PipeManagerTyped::pushTarget(CallbackT in, NodeType const& send_to_node) {
  auto const& han = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return std::tuple_cat(
    std::make_tuple(callback::CallbackSend<MsgT>(han,send_to_node)), in
  );
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
auto PipeManagerTyped::pushTarget(NodeType const& send_to_node) {
  auto const& han = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return std::make_tuple(callback::CallbackSend<MsgT>(han,send_to_node));
}

template <typename CallbackT>
auto PipeManagerTyped::buildMultiCB(CallbackT in) {
  using MsgT = typename std::tuple_element<0,CallbackT>::type::SignalDataType;
  auto const& new_pipe_id = makePipeID(true,false);
  return interface::CallbackDirectSendMulti<MsgT,CallbackT>(
    interface::CallbackDirectSendMultiTag,new_pipe_id,in
  );
}

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_TYPED_IMPL_H*/
