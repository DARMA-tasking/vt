/*
//@HEADER
// ************************************************************************
//
//                          auto_registry_functor_impl.h
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
#if !defined INCLUDED_REGISTRY_AUTO_FUNCTOR_AUTO_REGISTRY_FUNCTOR_IMPL_H
#define INCLUDED_REGISTRY_AUTO_FUNCTOR_AUTO_REGISTRY_FUNCTOR_IMPL_H

#include "vt/config.h"
#include "vt/registry/auto/functor/auto_registry_functor.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/utils/demangle/demangle.h"

#include <vector>
#include <memory>
#include <cassert>

namespace vt { namespace auto_registry {

template <typename FunctorT, bool msg, typename... Args>
inline HandlerType makeAutoHandlerFunctor() {
  using ContainerType = AutoActiveFunctorContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveFunctorType>;
  using FuncType = ActiveFnPtrType;
  using RunType = RunnableFunctor<
    FunctorT, ContainerType, RegInfoType, FuncType, msg, Args...
  >;
  return HandlerManagerType::makeHandler(true, true, RunType::idx);
}

template <typename FunctorT, typename... Args>
static inline auto functorHandlerWrapperRval(Args&&... args) {
  typename FunctorT::FunctorType instance;
  return instance.operator()(std::forward<Args>(args)...);
}

template <typename FunctorT, typename... Args>
static inline auto functorHandlerWrapperReg(Args... args) {
  typename FunctorT::FunctorType instance;
  return instance.operator()(args...);
}

template <
  typename FunctorT, typename RegT, typename InfoT, typename FnT,
  typename... Args
>
static inline void pullApart(
  RegT& reg, bool const& msg,
  pack<Args...> __attribute__((unused)) packed_args
) {
  #if backend_check_enabled(trace_enabled)
  using DemangleType = util::demangle::DemanglerUtils;
  auto const& name = DemangleType::getTypeName<typename FunctorT::FunctorType>();
  auto const& args = DemangleType::getTypeName<pack<Args...>>();
  auto const& parsed_type_name =
    util::demangle::ActiveFunctorDemangler::parseActiveFunctorName(name, args);
  auto const& trace_ep = trace::TraceRegistry::registerEventHashed(
    parsed_type_name.getNamespace(), parsed_type_name.getFuncParams()
  );
  #endif

  using TupleType = std::tuple<Args...>;
  static constexpr auto num_args = std::tuple_size<TupleType>::value;

  if (msg) {
    auto fn_ptr = functorHandlerWrapperReg<FunctorT, Args...>;
    reg.emplace_back(InfoT{
      NumArgsTag,
      reinterpret_cast<FnT>(fn_ptr)
      #if backend_check_enabled(trace_enabled)
      , trace_ep
      #endif
      , num_args
    });
  } else {
    auto fn_ptr = functorHandlerWrapperRval<FunctorT, Args...>;
    reg.emplace_back(InfoT{
      NumArgsTag,
      reinterpret_cast<FnT>(fn_ptr)
      #if backend_check_enabled(trace_enabled)
      , trace_ep
      #endif
     , num_args
    });
  }
}

template <typename FunctorT, typename RegT, typename InfoT, typename FnT>
RegistrarFunctor<FunctorT, RegT, InfoT, FnT>::RegistrarFunctor() {
  auto& reg = getAutoRegistryGen<RegT>();
  index = reg.size();

  pullApart<FunctorT, RegT, InfoT, FnT>(
    reg, FunctorT::IsMsgType, typename FunctorT::PackedArgsType()
  );
}

inline NumArgsType getAutoHandlerFunctorArgs(HandlerType const& han) {
  auto const& id = HandlerManagerType::getHandlerIdentifier(han);

  using ContainerType = AutoActiveFunctorContainerType;
  return getAutoRegistryGen<ContainerType>().at(id).getNumArgs();
}

inline AutoActiveFunctorType getAutoHandlerFunctor(HandlerType const& han) {
  auto const& id = HandlerManagerType::getHandlerIdentifier(han);
  bool const& is_auto = HandlerManagerType::isHandlerAuto(han);
  bool const& is_functor = HandlerManagerType::isHandlerFunctor(han);

  debug_print(
    handler, node,
    "getAutoHandlerFunctor: handler={}, id={}, is_auto={}, is_functor={}\n",
    han, id, print_bool(is_auto), print_bool(is_functor)
  );

  assert(
    (is_functor && is_auto) && "Handler should be auto and functor type!"
  );

  using ContainerType = AutoActiveFunctorContainerType;
  return getAutoRegistryGen<ContainerType>().at(id).getFun();
}

template <typename FunctorT, typename RegT, typename InfoT, typename FnT>
AutoHandlerType registerActiveFunctor() {
  return RegistrarWrapperFunctor<FunctorT, RegT, InfoT, FnT>().registrar.index;
}

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_FUNCTOR_AUTO_REGISTRY_FUNCTOR_IMPL_H*/
