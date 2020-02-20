/*
//@HEADER
// *****************************************************************************
//
//                             auto_registry_impl.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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
#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_IMPL_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_IMPL_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry.h"
#include "vt/utils/demangle/demangle.h"
#include "vt/objgroup/active_func/active_func.h"

#include <vector>
#include <memory>

namespace vt { namespace auto_registry {

inline AutoActiveObjGroupType getAutoHandlerObjGroup(HandlerType han) {
  using ContainerType = AutoActiveObjGroupContainerType;
  auto const id = HandlerManagerType::getHandlerIdentifier(han);
  return getAutoRegistryGen<ContainerType>().at(id).getFun();
}

inline AutoHandlerType getAutoHandlerObjTypeIdx(HandlerType han) {
  using ContainerType = AutoActiveObjGroupContainerType;
  auto const id = HandlerManagerType::getHandlerIdentifier(han);
  return getAutoRegistryGen<ContainerType>().at(id).getObjIdx();
}

template <typename ObjT, typename MsgT, objgroup::ActiveObjType<MsgT, ObjT> f>
inline HandlerType makeAutoHandlerObjGroup(HandlerControlType ctrl) {
  using AdapterT = FunctorAdapterMember<
    objgroup::ActiveObjType<MsgT, ObjT>, f, ObjT
  >;
  using ContainerType = AutoActiveObjGroupContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveObjGroupType>;
  using FuncType = objgroup::ActiveObjAnyType;
  using RunType = RunnableGen<AdapterT, ContainerType, RegInfoType, FuncType>;

  auto const obj = true;
  auto const idx = RunType::idx;
  auto const han = HandlerManagerType::makeHandler(true, false, idx, obj, ctrl);
  auto obj_idx = objgroup::registry::makeObjIdx<ObjT>();
  getAutoRegistryGen<ContainerType>().at(idx).setObjIdx(obj_idx);
  return han;
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
inline HandlerType makeAutoHandler(MessageT* const __attribute__((unused)) msg) {
  using AdapterT = FunctorAdapter<ActiveTypedFnType<MessageT>, f>;
  using ContainerType = AutoActiveContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveType>;
  using FuncType = ActiveFnPtrType;
  using RunType = RunnableGen<AdapterT, ContainerType, RegInfoType, FuncType>;

  return HandlerManagerType::makeHandler(true, false, RunType::idx);
}

template <typename T, T value>
inline HandlerType makeAutoHandlerParam() {
  using AdapterT = FunctorAdapter<T, value>;
  using ContainerType = AutoActiveContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveType>;
  using FuncType = ActiveFnPtrType;
  using RunType = RunnableGen<AdapterT, ContainerType, RegInfoType, FuncType>;

  return HandlerManagerType::makeHandler(true, false, RunType::idx);
}

inline AutoActiveType getAutoHandler(HandlerType const& handler) {
  auto const& han_id = HandlerManagerType::getHandlerIdentifier(handler);
  bool const& is_auto = HandlerManagerType::isHandlerAuto(handler);
  bool const& is_functor = HandlerManagerType::isHandlerFunctor(handler);

  debug_print(
    handler, node,
    "get_auto_handler: handler={}, id={}, is_auto={}, is_functor={}\n",
    handler, han_id, print_bool(is_auto), print_bool(is_functor)
  );

  assert(
    not is_functor and is_auto and "Handler should not be a functor, but auto"
  );

  return getAutoRegistryGen<AutoActiveContainerType>().at(han_id).getFun();
}

template <typename ObjT, typename MsgT, objgroup::ActiveObjType<MsgT, ObjT> f>
void setHandlerTraceNameObjGroup(
  HandlerControlType ctrl, std::string const& name, std::string const& parent
) {
#if backend_check_enabled(trace_enabled)
  auto const handler = makeAutoHandlerObjGroup<ObjT,MsgT,f>(ctrl);
  auto const trace_id = handlerTraceID(handler, RegistryTypeEnum::RegObjGroup);
  trace::TraceRegistry::setTraceName(trace_id, name, parent);
#endif
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
void setHandlerTraceName(std::string const& name, std::string const& parent) {
#if backend_check_enabled(trace_enabled)
  auto const handler = makeAutoHandler<MsgT,f>(nullptr);
  auto const trace_id = handlerTraceID(handler, RegistryTypeEnum::RegGeneral);
  trace::TraceRegistry::setTraceName(trace_id, name, parent);
#endif
}

template <typename T, T value>
void setHandlerTraceName(std::string const& name, std::string const& parent) {
#if backend_check_enabled(trace_enabled)
  auto const handler = makeAutoHandlerParam<T,value>();
  auto const trace_id = handlerTraceID(handler, RegistryTypeEnum::RegGeneral);
  trace::TraceRegistry::setTraceName(trace_id, name, parent);
#endif
}

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_IMPL_H*/
