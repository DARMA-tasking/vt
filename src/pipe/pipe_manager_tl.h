
#if !defined INCLUDED_PIPE_PIPE_MANAGER_TL_H
#define INCLUDED_PIPE_PIPE_MANAGER_TL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/pipe_manager_base.h"
#include "pipe/callback/cb_union/cb_raw_base.h"
#include "activefn/activefn.h"
#include "vrt/collection/active/active_funcs.h"
#include "vrt/proxy/collection_proxy.h"

namespace vt { namespace pipe {

struct PipeManagerTL : virtual PipeManagerBase {
  template <typename ColT, typename MsgT>
  using ColHanType = vrt::collection::ActiveColTypedFnType<MsgT,ColT>;

  template <typename ColT>
  using ColProxyType = CollectionProxy<ColT,typename ColT::IndexType>;

  using CallbackType = callback::cbunion::CallbackRawBaseSingle;

  template <typename MsgT>
  using CallbackMsgType = callback::cbunion::CallbackTyped<MsgT>;

  /*
   *  Untyped variants of callbacks: uses union to dispatch
   */

  // Single active message function-handler
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  CallbackType makeCallbackSingleSend(NodeType const& node);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  CallbackType makeCallbackSingleBcast(bool const& inc);

  // Single active message functor-handler
  template <typename FunctorT>
  CallbackType makeCallbackFunctorSend(NodeType const& node);

  template <typename FunctorT>
  CallbackType makeCallbackFunctorBcast(bool const& inc);

  // Single active message functor-handler void param
  template <typename FunctorT>
  CallbackType makeCallbackFunctorSendVoid(NodeType const& node);

  template <typename FunctorT>
  CallbackType makeCallbackFunctorBcastVoid(bool const& inc);

  // Single active message anon func-handler
  template <typename=void>
  CallbackType makeCallbackSingleAnonVoid(FuncVoidType fn);

  template <typename MsgT>
  CallbackType makeCallbackSingleAnon(FuncMsgType<MsgT> fn);

  // Single active message collection proxy send
  template <typename ColT, typename MsgT, ColHanType<ColT,MsgT>* f>
  CallbackType makeCallbackSingleProxySend(typename ColT::ProxyType proxy);

  // Single active message collection proxy bcast
  template <typename ColT, typename MsgT, ColHanType<ColT,MsgT>* f>
  CallbackType makeCallbackSingleProxyBcast(ColProxyType<ColT> proxy);

  // Single active message collection proxy bcast direct
  template <typename ColT, typename MsgT, ColHanType<ColT,MsgT>* f>
  CallbackType makeCallbackSingleProxyBcastDirect(ColProxyType<ColT> proxy);

  // Multi-staged callback
  template <typename=void>
  CallbackType makeCallback();

  template <typename T>
  CallbackMsgType<T> makeCallbackTyped();

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  void addListener(CallbackType const& cb, NodeType const& node);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  void addListenerBcast(CallbackType const& cb, bool const& inc);

  template <typename FunctorT>
  void addListenerFunctor(CallbackType const& cb, NodeType const& node);

  template <typename FunctorT>
  void addListenerFunctorVoid(CallbackType const& cb, NodeType const& node);

  template <typename FunctorT>
  void addListenerFunctorBcast(CallbackType const& cb, bool const& inc);
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_TL_H*/
