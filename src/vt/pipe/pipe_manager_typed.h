
#if !defined INCLUDED_PIPE_PIPE_MANAGER_TYPED_H
#define INCLUDED_PIPE_PIPE_MANAGER_TYPED_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager_base.h"
#include "vt/pipe/pipe_manager_typed.fwd.h"
#include "vt/pipe/interface/send_container.h"
#include "vt/pipe/interface/callback_direct.h"
#include "vt/pipe/interface/callback_direct_multi.h"
#include "vt/pipe/interface/callback_types.h"
#include "vt/pipe/callback/handler_send/callback_send_han.h"
#include "vt/pipe/pipe_manager_construct.h"
#include "vt/utils/static_checks/functor.h"
#include "vt/vrt/collection/active/active_funcs.h"

#include <type_traits>
#include <tuple>
#include <functional>

namespace vt { namespace pipe {

struct PipeManagerTyped : virtual PipeManagerBase {
  template <typename T>
  using CBTypes                = interface::CallbackTypes<T>;
  template <typename T>
  using CallbackSendType       = typename CBTypes<T>::CallbackDirectSend;
  template <typename T>
  using CallbackAnonType       = typename CBTypes<T>::CallbackAnon;
  template <typename T>
  using CallbackBcastType      = typename CBTypes<T>::CallbackDirectBcast;
  using CallbackSendVoidType   = typename CBTypes<void>::CallbackDirectVoidSend;
  using CallbackBcastVoidType  = typename CBTypes<void>::CallbackDirectVoidBcast;
  using CallbackAnonVoidType   = typename CBTypes<void>::CallbackAnonVoid;

  template <typename C, typename T>
  using CBVrtTypes             = interface::CallbackVrtTypes<C,T>;
  template <typename C, typename T>
  using CallbackProxyBcastType = typename CBVrtTypes<C,T>::CallbackProxyBcast;
  template <typename C, typename T>
  using CallbackProxySendType  = typename CBVrtTypes<C,T>::CallbackProxySend;

  /*
   *  Builders for non-send-back type of pipe callback: they are invoked
   *  directly from the sender; thus this node is not involved in the process
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>*... f>
  interface::CallbackDirectSendMulti<
    MsgT,
    typename RepeatNImpl<sizeof...(f),callback::CallbackSend<MsgT>>::ResultType
  >
  makeCallbackMultiSendTyped(
    bool const is_persist, NodeType const& send_to_node
  );

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  auto pushTargetBcast(bool const& inc);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
  auto pushTargetBcast(CallbackT in, bool const& inc);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  auto pushTarget(NodeType const& send_to_node);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
  auto pushTarget(CallbackT in, NodeType const& send_to_node);

  template <typename CallbackT>
  auto buildMultiCB(CallbackT in);

  /*
   *  Make a fully typed single-endpoint listener callback that directly sends
   *  to the endpoint
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  CallbackSendType<MsgT> makeCallbackSingleSendTyped(
    bool const is_persist, NodeType const& send_to_node
  );

  template <
    typename FunctorT,
    typename MsgT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  CallbackSendType<MsgT> makeCallbackSingleSendFunctorTyped(
    bool const is_persist, NodeType const& send_to_node
  );

  template <typename FunctorT>
  CallbackSendVoidType makeCallbackSingleSendFunctorVoidTyped(
    bool const is_persist, NodeType const& send_to_node
  );

  /*
   *  Make a fully typed single-endpoint anon function callback
   */
  template <typename MsgT>
  CallbackAnonType<MsgT> makeCallbackSingleAnonTyped(
    bool const persist, FuncMsgType<MsgT> fn
  );

  template <typename=void>
  CallbackAnonVoidType makeCallbackSingleAnonVoidTyped(
    bool const persist, FuncVoidType fn
  );

  /*
   *  Make a fully typed single-endpoint broadcast directly routed callback
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  CallbackBcastType<MsgT> makeCallbackSingleBcastTyped(bool const inc);

  template <
    typename FunctorT,
    typename MsgT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  CallbackBcastType<MsgT> makeCallbackSingleBcastFunctorTyped(bool const inc);

  template <typename FunctorT>
  CallbackBcastVoidType makeCallbackSingleBcastFunctorVoidTyped(bool const inc);

  /*
   *  Make a fully typed vrt proxy broadcast directly routed callback
   */
  template <
    typename ColT,
    typename MsgT,
    vrt::collection::ActiveColTypedFnType<MsgT,ColT> *f
  >
  CallbackProxyBcastType<ColT,MsgT> makeCallbackSingleProxyBcastTyped(
    CollectionProxy<ColT,typename ColT::IndexType> proxy
  );

  /*
   *  Make a fully typed vrt proxy indexed send directly routed callback
   */
  template <
    typename ColT,
    typename MsgT,
    vrt::collection::ActiveColTypedFnType<MsgT,ColT> *f
  >
  CallbackProxySendType<ColT,MsgT> makeCallbackSingleProxySendTyped(
    typename ColT::ProxyType proxy
  );
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_TYPED_H*/
