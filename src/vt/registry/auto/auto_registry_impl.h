/*
//@HEADER
// ************************************************************************
//
//                          auto_registry_impl.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
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
  return getAutoRegistryGen<ContainerType>().at(han).getFun();
}

template <typename ObjT, typename MsgT, objgroup::ActiveObjType<MsgT, ObjT> f>
inline HandlerType makeAutoHandlerObjGroup() {
  using FunctorT = FunctorAdapterMember<objgroup::ActiveObjType<MsgT, ObjT>, f>;
  using ContainerType = AutoActiveObjGroupContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveObjGroupType>;
  using FuncType = objgroup::ActiveObjAnyType;
  using RunType = RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>;
  auto const is_obj = true;
  return HandlerManagerType::makeHandler(true, false, RunType::idx, is_obj);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
inline HandlerType makeAutoHandler(MessageT* const __attribute__((unused)) msg) {
  using FunctorT = FunctorAdapter<ActiveTypedFnType<MessageT>, f>;
  using ContainerType = AutoActiveContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveType>;
  using FuncType = ActiveFnPtrType;
  using RunType = RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>;
  //auto const& name = demangle::DemanglerUtils::getDemangledType<FunctorT>();
  return HandlerManagerType::makeHandler(true, false, RunType::idx);
}

template <typename T, T value>
inline HandlerType makeAutoHandler() {
  using FunctorT = FunctorAdapter<T, value>;
  using ContainerType = AutoActiveContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveType>;
  using FuncType = ActiveFnPtrType;
  using RunType = RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>;
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

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_IMPL_H*/
