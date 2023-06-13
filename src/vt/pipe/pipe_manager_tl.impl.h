/*
//@HEADER
// *****************************************************************************
//
//                            pipe_manager_tl.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_PIPE_PIPE_MANAGER_TL_IMPL_H
#define INCLUDED_VT_PIPE_PIPE_MANAGER_TL_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager_tl.h"
#include "vt/pipe/pipe_manager_base.h"
#include "vt/pipe/callback/cb_union/cb_raw_base.h"
#include "vt/pipe/callback/callback_base.h"
#include "vt/pipe/callback/handler_send/callback_send.h"
#include "vt/pipe/callback/handler_bcast/callback_bcast.h"
#include "vt/pipe/callback/proxy_send/callback_proxy_send.h"
#include "vt/pipe/callback/proxy_bcast/callback_proxy_bcast.h"
#include "vt/pipe/callback/proxy_send/callback_proxy_send_tl.h"
#include "vt/pipe/callback/proxy_bcast/callback_proxy_bcast_tl.h"
#include "vt/pipe/callback/objgroup_send/callback_objgroup_send.h"
#include "vt/pipe/callback/objgroup_bcast/callback_objgroup_bcast.h"
#include "vt/activefn/activefn.h"
#include "vt/context/context.h"
#include "vt/utils/static_checks/functor.h"
#include "vt/registry/auto/collection/auto_registry_collection.h"
#include "vt/vrt/collection/dispatch/registry.h"
#include "vt/objgroup/proxy/proxy_bits.h"
#include "vt/utils/fntraits/fntraits.h"
#include "vt/messaging/param_msg.h"
#include "vt/vrt/collection/messages/param_col_msg.h"

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

template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
void PipeManagerTL::addListener(CallbackT const& cb, NodeType const& node) {
  auto const& han = auto_registry::makeAutoHandler<MsgT,f>();
  addListenerAny<MsgT>(
    cb.getPipe(), std::make_unique<callback::CallbackSend<MsgT>>(han,node)
  );
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
void PipeManagerTL::addListenerBcast(CallbackT const& cb) {
  auto const& han = auto_registry::makeAutoHandler<MsgT,f>();
  addListenerAny<MsgT>(
    cb.getPipe(), std::make_unique<callback::CallbackBcast<MsgT>>(han)
  );
}

template <typename FunctorT, typename T, typename CallbackT>
void PipeManagerTL::addListenerFunctor(
  CallbackT const& cb, NodeType const& node
) {
  using MsgT = typename util::FunctorExtractor<FunctorT>::MessageType;
  auto const& han = auto_registry::makeAutoHandlerFunctor<FunctorT, MsgT, true>();
  addListenerAny<MsgT>(
    cb.getPipe(), std::make_unique<callback::CallbackSend<MsgT>>(han,node)
  );
}

template <typename FunctorT, typename CallbackT>
void PipeManagerTL::addListenerFunctorVoid(
  CallbackT const& cb, NodeType const& node
) {
  auto const& han = auto_registry::makeAutoHandlerFunctor<FunctorT, void, false>();
  addListenerAny<signal::SigVoidType>(
    cb.getPipe(), std::make_unique<callback::CallbackSend<signal::SigVoidType>>(
      han,node
    )
  );
}

template <typename FunctorT, typename T, typename CallbackT>
void PipeManagerTL::addListenerFunctorBcast(
  CallbackT const& cb
) {
  using MsgT = typename util::FunctorExtractor<FunctorT>::MessageType;
  auto const& han = auto_registry::makeAutoHandlerFunctor<FunctorT, MsgT, true>();
  addListenerAny<MsgT>(
    cb.getPipe(), std::make_unique<callback::CallbackBcast<MsgT>>(han)
  );
}

template <typename U>
using hasIdx_t = typename U::IndexType;

template <auto f, bool is_bcast, typename ProxyT>
auto PipeManagerTL::makeCallbackProxy(ProxyT proxy) {
  bool const persist = true;
  bool const send_back = false;
  bool const dispatch = true;
  auto const pipe_id = makePipeID(persist, send_back);
  newPipeState(pipe_id, persist, dispatch, -1, -1, 0);

  using Trait = ObjFuncTraits<decltype(f)>;
  using ObjT = typename Trait::ObjT;
  using MsgT = typename Trait::MsgT;
  using RetType = typename Trait::template WrapType<CallbackRetType>;

  if constexpr (detection::is_detected<hasIdx_t, ProxyT>::value) {
    if constexpr (std::is_same_v<MsgT, NoMsg>) {
      using Tuple = typename Trait::TupleType;
      using NormMsgT = messaging::ParamMsg<Tuple>;
      using WrapMsgT = vrt::collection::ColMsgWrap<ObjT, NormMsgT, vt::Message>;
      auto han = auto_registry::makeAutoHandlerCollectionMemParam<
        ObjT, decltype(f), f, WrapMsgT
      >();
      if constexpr (is_bcast) {
        auto vrt_handler = vrt::collection::makeVrtDispatch<NormMsgT, ObjT>();
        return RetType{
          callback::cbunion::RawBcastColDirTag, pipe_id, han, vrt_handler,
          proxy.getProxy()
        };
      } else {
        auto cb = RetType(callback::cbunion::RawSendColMsgTag, pipe_id);
        addListenerAny<NormMsgT>(
          cb.getPipe(),
          std::make_unique<callback::CallbackProxySend<ObjT, NormMsgT>>(han, proxy)
        );
        return cb;
      }
    } else {
      HandlerType han = uninitialized_handler;
      if constexpr (Trait::is_member) {
        han = auto_registry::makeAutoHandlerCollectionMem<ObjT, MsgT, f>();
      } else {
        han = auto_registry::makeAutoHandlerCollection<ObjT, MsgT, f>();
      }
      if constexpr (is_bcast) {
        auto vrt_handler = vrt::collection::makeVrtDispatch<MsgT, ObjT>();
        return RetType{
          callback::cbunion::RawBcastColDirTag, pipe_id, han, vrt_handler,
          proxy.getProxy()
        };
      } else {
        auto cb = RetType(callback::cbunion::RawSendColMsgTag, pipe_id);
        addListenerAny<MsgT>(
          cb.getPipe(),
          std::make_unique<callback::CallbackProxySend<ObjT, MsgT>>(han, proxy)
        );
        return cb;
      }
    }
  } else {
    auto const proxy_bits = proxy.getProxy();
    auto const ctrl = objgroup::proxy::ObjGroupProxy::getID(proxy_bits);
    HandlerType han = uninitialized_handler;
    if constexpr (std::is_same_v<MsgT, NoMsg>) {
      using Tuple = typename Trait::TupleType;
      using PMsgT = messaging::ParamMsg<Tuple>;
      han = auto_registry::makeAutoHandlerObjGroupParam<
        ObjT, decltype(f), f, PMsgT
      >(ctrl);
    } else {
      han = auto_registry::makeAutoHandlerObjGroup<ObjT,MsgT,f>(ctrl);
    }
    if constexpr (is_bcast) {
      return RetType{
        callback::cbunion::RawBcastObjGrpTag, pipe_id, han, proxy_bits
      };
    } else {
      auto const dest_node = proxy.getNode();
      return RetType{
        callback::cbunion::RawSendObjGrpTag, pipe_id, han, proxy_bits, dest_node
      };
    }
  }

  // unnecessary, but to make nvcc happy
  return RetType{};
}

template <auto f, bool is_bcast>
auto PipeManagerTL::makeCallbackSingle(NodeType node) {
  auto const new_pipe_id = makePipeID(true,false);

  using Trait = FuncTraits<decltype(f)>;
  using MsgT = typename Trait::MsgT;

  HandlerType han = uninitialized_handler;
  if constexpr (std::is_same_v<MsgT, NoMsg>) {
    using SendMsgT = messaging::ParamMsg<typename Trait::TupleType>;
    han = auto_registry::makeAutoHandlerParam<decltype(f), f, SendMsgT>();
  } else {
    han = auto_registry::makeAutoHandler<MsgT,f>();
  }

  using RetType = typename Trait::template WrapType<CallbackRetType>;
  if constexpr (is_bcast) {
    return RetType{callback::cbunion::RawBcastMsgTag, new_pipe_id, han};
  } else {
    return RetType{callback::cbunion::RawSendMsgTag, new_pipe_id, han, node};
  }

  // unnecessary, but to make nvcc happy
  return RetType{};
}

template <typename FunctorT, bool is_bcast>
auto PipeManagerTL::makeCallbackFunctor(NodeType node) {
  auto const new_pipe_id = makePipeID(true,false);

  using Trait = FunctorTraits<FunctorT, decltype(&FunctorT::operator())>;
  using MsgT = typename Trait::MsgT;

  HandlerType han = uninitialized_handler;
  if constexpr (std::is_same_v<MsgT, NoMsg>) {
    using SendMsgT = messaging::ParamMsg<typename Trait::TupleType>;
    han = auto_registry::makeAutoHandlerFunctor<FunctorT, SendMsgT, false>();
  } else {
    han = auto_registry::makeAutoHandlerFunctor<FunctorT, MsgT, true>();
  }

  using RetType = typename Trait::template WrapType<CallbackRetType>;
  if constexpr (is_bcast) {
    return RetType{callback::cbunion::RawBcastMsgTag, new_pipe_id, han};
  } else {
    return RetType{callback::cbunion::RawSendMsgTag, new_pipe_id, han, node};
  }

  // unnecessary, but to make nvcc happy
  return RetType{};
}

inline auto PipeManagerTL::makeCallbackSingleAnonVoid(LifetimeEnum life, FuncVoidType fn) {
  PipeType new_pipe_id = no_pipe;
  if (life == LifetimeEnum::Once) {
    new_pipe_id = makeCallbackFuncVoid(false,fn,true,1,1);
  } else {
    new_pipe_id = makeCallbackFuncVoid(true,fn,true);
  }
  auto cb = CallbackRetType<>(callback::cbunion::RawAnonTag,new_pipe_id);

  vt_debug_print(
    normal, pipe,
    "makeCallbackSingleAnonVoid: persist={}, pipe={:x}\n",
    true, new_pipe_id
  );

  return cb;
}

template <typename C>
auto PipeManagerTL::makeCallbackSingleAnon(
  LifetimeEnum life, C* ctx, FuncCtxType<C> fn
) {
  auto fn_closure = [ctx,fn] { fn(ctx); };

  vt_debug_print(
    normal, pipe,
    "makeCallbackSingleAnon: created closure\n"
  );

  return makeCallbackSingleAnonVoid(life,fn_closure);
}

template <typename MsgT, typename C>
auto PipeManagerTL::makeCallbackSingleAnon(
  LifetimeEnum life, C* ctx, FuncMsgCtxType<MsgT, C> fn
) {
  auto fn_closure = [ctx,fn](MsgT* msg) { fn(msg, ctx); };

  vt_debug_print(
    normal, pipe,
    "makeCallbackSingleAnon: created closure\n"
  );

  return makeCallbackSingleAnon<MsgT>(life,fn_closure);
}

template <typename MsgT>
auto PipeManagerTL::makeCallbackSingleAnon(LifetimeEnum life, FuncMsgType<MsgT> fn) {
  PipeType new_pipe_id = no_pipe;
  if (life == LifetimeEnum::Once) {
    new_pipe_id = makeCallbackFunc<MsgT>(false,fn,true,1,1);
  } else {
    new_pipe_id = makeCallbackFunc<MsgT>(true,fn,true);
  }

  auto cb = CallbackRetType<MsgT>(callback::cbunion::RawAnonTag,new_pipe_id);

  vt_debug_print(
    normal, pipe,
    "makeCallbackSingleAnon: persist={}, pipe={:x}\n",
    true, new_pipe_id
  );

  return cb;
}

}} /* end namespace vt::pipe */

#endif /*INCLUDED_VT_PIPE_PIPE_MANAGER_TL_IMPL_H*/
