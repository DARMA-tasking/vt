/*
//@HEADER
// *****************************************************************************
//
//                            auto_registry_common.h
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

#if !defined INCLUDED_VT_REGISTRY_AUTO_AUTO_REGISTRY_COMMON_H
#define INCLUDED_VT_REGISTRY_AUTO_AUTO_REGISTRY_COMMON_H

#include "vt/trace/trace_event.h"

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/registry/registry.h"
#include "vt/trace/trace.h"
#include "vt/vrt/context/context_vrt_funcs.h"
#include "vt/vrt/collection/active/active_funcs.h"
#include "vt/topos/mapping/mapping_function.h"
#include "vt/objgroup/active_func/active_func.h"

#include <vector>
#include <cstdlib>
#include <functional>

namespace vt { namespace auto_registry {

struct SentinelObject {};

struct BaseHandlersDispatcher {
  virtual ~BaseHandlersDispatcher() = default;
  virtual void dispatch(messaging::BaseMsg* msg, void* object) const = 0;
};

template <typename MsgT, typename HandlerT, typename ObjT>
struct HandlersDispatcher final : BaseHandlersDispatcher {
  explicit HandlersDispatcher(HandlerT in_fn_ptr) : fn_ptr_(in_fn_ptr) { }

private:
  template <typename T, typename = void>
  struct DispatchImpl;

  template <typename T>
  using isActiveVoidFnType = std::enable_if_t<
    std::is_same<
      T,
      ActiveVoidFnType*
    >::value
  >;

  template <typename T>
  struct DispatchImpl<T, isActiveVoidFnType<T>> {
    static void run(MsgT*, void*, HandlerT han) { han(); }
  };

  template <typename T>
  using isActiveTypedFnType = std::enable_if_t<
    std::is_same<
      T,
      ActiveTypedFnType<MsgT>*
    >::value
  >;

  template <typename T>
  struct DispatchImpl<T, isActiveTypedFnType<T>> {
    static void run(MsgT* msg, void*, HandlerT han) { han(msg); }
  };

  template <typename T>
  using isActiveColTypedFnType = std::enable_if_t<
    std::is_same<
      T,
      vrt::collection::ActiveColTypedFnType<MsgT, ObjT>*
    >::value
  >;

  template <typename T>
  struct DispatchImpl<T, isActiveColTypedFnType<T>> {
    static void run(MsgT* msg, void* object, HandlerT han) {
      auto elm = static_cast<ObjT*>(object);
      han(msg, elm);
    }
  };

  template <typename T>
  using isActiveColMemberTypedFnType = std::enable_if_t<
    std::is_same<
      T,
      vrt::collection::ActiveColMemberTypedFnType<MsgT, ObjT>
    >::value
  >;

  template <typename T>
  struct DispatchImpl<T, isActiveColMemberTypedFnType<T>> {
    static void run(MsgT* msg, void* object, HandlerT han) {
      auto elm = static_cast<ObjT*>(object);
      (elm->*han)(msg);
    }
  };

public:
  void dispatch(messaging::BaseMsg* msg, void* object) const override {
    DispatchImpl<HandlerT>::run(static_cast<MsgT*>(msg), object, fn_ptr_);
  }

private:
  HandlerT fn_ptr_ = nullptr;
};

struct BaseMapsDispatcher {
  virtual ~BaseMapsDispatcher() = default;
  virtual NodeType dispatch(
    index::BaseIndex* cur_idx_ptr,
    index::BaseIndex* range_ptr,
    NodeType num_nodes
  ) const = 0;
};

template <typename IndexT, typename HandlerT>
struct MapsDispatcher final : BaseMapsDispatcher {
  explicit MapsDispatcher(HandlerT in_fn_ptr) : fn_ptr_(in_fn_ptr) { }

private:
  template <typename T, typename = void>
  struct DispatchImpl;

  template <typename T>
  using isActiveMapFnPtrType = std::enable_if_t<
    std::is_same<
      T,
      vt::mapping::ActiveMapTypedFnType<IndexT>*
    >::value
  >;

  template <typename T>
  struct DispatchImpl<T, isActiveMapFnPtrType<T>> {
    static NodeType run(
      IndexT* cur_idx_ptr,
      IndexT* range_ptr,
      NodeType num_nodes,
      HandlerT han
    ) {
      return han(cur_idx_ptr, range_ptr, num_nodes);
    }
  };

public:
  NodeType dispatch(
    index::BaseIndex* cur_idx_ptr,
    index::BaseIndex* range_ptr,
    NodeType num_nodes
  ) const override {
    return DispatchImpl<HandlerT>::run(
      static_cast<IndexT*>(cur_idx_ptr),
      static_cast<IndexT*>(range_ptr),
      num_nodes,
      fn_ptr_
    );
  }

private:
  HandlerT fn_ptr_ = nullptr;
};

struct BaseScatterDispatcher {
  virtual ~BaseScatterDispatcher() = default;
  virtual void dispatch(void* msg, void* object) const = 0;
};

template <typename MsgT, typename HandlerT, typename ObjT>
struct ScatterDispatcher final : BaseScatterDispatcher {
  explicit ScatterDispatcher(HandlerT in_fn_ptr) : fn_ptr_(in_fn_ptr) {}

private:
  template <typename T, typename = void>
  struct DispatchImpl;

  template <typename T>
  using isActiveTypedFnType = std::enable_if_t<
    std::is_same<
      T,
      ActiveTypedFnType<MsgT>*
    >::value
  >;

  template <typename T>
  struct DispatchImpl<T, isActiveTypedFnType<T>> {
    static void run(MsgT* msg, void*, HandlerT han) { han(msg); }
  };

public:
  void dispatch(void* msg, void* object) const override {
    DispatchImpl<HandlerT>::run(static_cast<MsgT*>(msg), object, fn_ptr_);
  }

private:
  HandlerT fn_ptr_ = nullptr;
};

using BaseHandlersDispatcherPtr = std::unique_ptr<BaseHandlersDispatcher>;
using BaseMapsDispatcherPtr     = std::unique_ptr<BaseMapsDispatcher>;
using BaseScatterDispatcherPtr  = std::unique_ptr<BaseScatterDispatcher>;

using AutoActiveType              = BaseHandlersDispatcherPtr;
using AutoActiveFunctorType       = BaseHandlersDispatcherPtr;
using AutoActiveVCType            = BaseHandlersDispatcherPtr;
using AutoActiveCollectionType    = BaseHandlersDispatcherPtr;
using AutoActiveCollectionMemType = BaseHandlersDispatcherPtr;
using AutoActiveMapType           = BaseMapsDispatcherPtr;
using AutoActiveMapFunctorType    = BaseMapsDispatcherPtr;

using AutoActiveSeedMapType       = mapping::ActiveSeedMapFnPtrType;
using AutoActiveRDMAGetType       = ActiveRDMAGetFnPtrType;
using AutoActiveRDMAPutType       = ActiveRDMAPutFnPtrType;
using AutoActiveIndexType         = std::size_t;
using AutoActiveObjGroupType      = objgroup::ActiveObjAnyType;

using HandlerManagerType = vt::HandlerManager;
using AutoHandlerType = HandlerType;
using NumArgsType = int16_t;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static struct NumArgsTagType { } NumArgsTag { };
#pragma GCC diagnostic pop

struct RegistrarGenInfoBase {
  /// Return the registered handler type.
  virtual HandlerType getRegisteredIndex() = 0;
  virtual ~RegistrarGenInfoBase() {}
};

struct RegistrarGenInfo : RegistrarGenInfoBase {

  /// Create a new object.
  /// Takes complete ownership of the supplied object (pointer).
  static RegistrarGenInfo takeOwnership(RegistrarGenInfoBase *owned_proxy) {
    return RegistrarGenInfo(owned_proxy);
  }

  RegistrarGenInfo() {
  }

  // Using unique_ptr; can expand later.
  RegistrarGenInfo(RegistrarGenInfo const& in) = delete;

  RegistrarGenInfo(RegistrarGenInfo&& in) {
    proxy_.swap(in.proxy_);
  }

  virtual HandlerType getRegisteredIndex() override {
    return proxy_->getRegisteredIndex();
  }

private:
  explicit RegistrarGenInfo(RegistrarGenInfoBase* owned_proxy)
    : proxy_(owned_proxy)  {
  }

private:
  std::unique_ptr<RegistrarGenInfoBase> proxy_ = nullptr;
};

template <typename FnT>
struct AutoRegInfo {
  FnT activeFunT;
  NumArgsType args_ = 1;
  AutoHandlerType obj_idx_ = -1;
  RegistrarGenInfo gen_obj_idx_;

  #if vt_check_enabled(trace_enabled)
    trace::TraceEntryIDType event_id;
    AutoRegInfo(
      FnT in_active_fun_t,
      RegistrarGenInfo in_gen,
      trace::TraceEntryIDType const& in_event_id
    ) : activeFunT(std::move(in_active_fun_t)), gen_obj_idx_(std::move(in_gen)), event_id(in_event_id)
    { }
    AutoRegInfo(
      NumArgsTagType,
      FnT in_active_fun_t,
      trace::TraceEntryIDType const& in_event_id,
      NumArgsType const& in_args
    ) : activeFunT(std::move(in_active_fun_t)), args_(in_args), event_id(in_event_id)
    { }
    trace::TraceEntryIDType theTraceID() const {
      return event_id;
    }
  #else
    explicit AutoRegInfo(
      FnT in_active_fun_t,
      RegistrarGenInfo in_gen
    ) : activeFunT(std::move(in_active_fun_t)), gen_obj_idx_(std::move(in_gen))
    { }

    AutoRegInfo(
      NumArgsTagType,
      FnT in_active_fun_t,
      NumArgsType const& in_args
    ) : activeFunT(std::move(in_active_fun_t)), args_(in_args)
    { }
  #endif

  AutoHandlerType getObjIdx() {
    if (obj_idx_ == -1) {
      obj_idx_ = gen_obj_idx_.getRegisteredIndex();
    }
    return obj_idx_;
  }

  void setObjIdx(AutoHandlerType const obj_idx) {
    obj_idx_ = obj_idx;
  }

  NumArgsType getNumArgs() const {
    return args_;
  }

  FnT const& getFun() const {
    return activeFunT;
  }
};

template <typename Fn>
using AutoRegInfoType = AutoRegInfo<Fn>;

template <typename RegInfoT>
using RegContType = std::vector<AutoRegInfoType<RegInfoT>>;

using AutoActiveContainerType               = RegContType<AutoActiveType>;
using AutoActiveVCContainerType             = RegContType<AutoActiveVCType>;
using AutoActiveCollectionContainerType     = RegContType<AutoActiveCollectionType>;
using AutoActiveCollectionMemContainerType  = RegContType<AutoActiveCollectionMemType>;
using AutoActiveMapContainerType            = RegContType<AutoActiveMapType>;
using AutoActiveMapFunctorContainerType     = RegContType<AutoActiveMapFunctorType>;
using AutoActiveSeedMapContainerType        = RegContType<AutoActiveSeedMapType>;
using AutoActiveFunctorContainerType        = RegContType<AutoActiveFunctorType>;
using AutoActiveRDMAGetContainerType        = RegContType<AutoActiveRDMAGetType>;
using AutoActiveRDMAPutContainerType        = RegContType<AutoActiveRDMAPutType>;
using AutoActiveIndexContainerType          = RegContType<AutoActiveIndexType>;
using AutoActiveObjGroupContainerType       = RegContType<AutoActiveObjGroupType>;
using ScatterContainerType                  = RegContType<BaseScatterDispatcherPtr>;

}} // end namespace vt::auto_registry

#endif /*INCLUDED_VT_REGISTRY_AUTO_AUTO_REGISTRY_COMMON_H*/
