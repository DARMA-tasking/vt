
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

  template <typename MsgT>
  using FnType = ActiveTypedFnType<MsgT>;

  template <typename MsgT>
  using DefType = CallbackMsgType<MsgT>;

  using V = signal::SigVoidType;

  /*
   *  Untyped variants of callbacks: uses union to dispatch
   */


  template <typename T, FnType<T>* f, typename CbkT = DefType<T>>
  CbkT makeCallbackSingleSendT(NodeType const& node);

  // Single active message function-handler
  template <typename T, FnType<T>* f, typename CbkT = DefType<T>>
  CbkT makeCallbackSingleSend(NodeType const& node);

  template <typename T, FnType<T>* f, typename CbkT = DefType<T>>
  CbkT makeCallbackSingleBcast(bool const& inc);

  // Single active message functor-handler
  template <
    typename FunctorT,
    typename T = typename util::FunctorExtractor<FunctorT>::MessageType,
    typename CbkT = DefType<T>
  >
  CbkT makeCallbackFunctorSend(NodeType const& node);

  template <
    typename FunctorT,
    typename T = typename util::FunctorExtractor<FunctorT>::MessageType,
    typename CbkT = DefType<T>
  >
  CbkT makeCallbackFunctorBcast(bool const& inc);

  // Single active message functor-handler void param
  template <typename FunctorT, typename CbkT = DefType<V>>
  CbkT makeCallbackFunctorSendVoid(NodeType const& node);

  template <typename FunctorT, typename CbkT = DefType<V>>
  CbkT makeCallbackFunctorBcastVoid(bool const& inc);

  // Single active message anon func-handler
  template <typename CbkT = DefType<V>>
  CbkT makeCallbackSingleAnonVoid(FuncVoidType fn);

  template <typename T, typename CbkT = DefType<T>>
  CbkT makeCallbackSingleAnon(FuncMsgType<T> fn);

  // Single active message collection proxy send
  template <
    typename ColT, typename T, ColHanType<ColT,T>* f, typename CbkT = DefType<T>
  >
  CbkT makeCallbackSingleProxySend(typename ColT::ProxyType proxy);

  // Single active message collection proxy bcast
  template <
    typename ColT, typename T, ColHanType<ColT,T>* f, typename CbkT = DefType<T>
  >
  CbkT makeCallbackSingleProxyBcast(ColProxyType<ColT> proxy);

  // Single active message collection proxy bcast direct
  template <
    typename ColT, typename T, ColHanType<ColT,T>* f, typename CbkT = DefType<T>
  >
  CbkT makeCallbackSingleProxyBcastDirect(ColProxyType<ColT> proxy);

  // Multi-staged callback
  template <typename=void>
  CallbackType makeCallback();

  template <typename T>
  CallbackMsgType<T> makeCallbackTyped();

  template <typename T, ActiveTypedFnType<T>* f, typename CbkT = DefType<T>>
  void addListener(CbkT const& cb, NodeType const& node);

  template <typename T, ActiveTypedFnType<T>* f, typename CbkT = DefType<T>>
  void addListenerBcast(CbkT const& cb, bool const& inc);

  template <
    typename FunctorT,
    typename T = typename util::FunctorExtractor<FunctorT>::MessageType,
    typename CbkT = DefType<T>
  >
  void addListenerFunctor(CbkT const& cb, NodeType const& node);

  template <typename FunctorT, typename CbkT = DefType<V>>
  void addListenerFunctorVoid(CbkT const& cb, NodeType const& node);

  template <
    typename FunctorT,
    typename T = typename util::FunctorExtractor<FunctorT>::MessageType,
    typename CbkT = DefType<T>
  >
  void addListenerFunctorBcast(CbkT const& cb, bool const& inc);
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_TL_H*/
