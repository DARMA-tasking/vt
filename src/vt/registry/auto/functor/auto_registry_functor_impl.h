/*
//@HEADER
// *****************************************************************************
//
//                         auto_registry_functor_impl.h
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
#if !defined INCLUDED_REGISTRY_AUTO_FUNCTOR_AUTO_REGISTRY_FUNCTOR_IMPL_H
#define INCLUDED_REGISTRY_AUTO_FUNCTOR_AUTO_REGISTRY_FUNCTOR_IMPL_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/registry/auto/functor/auto_registry_functor.h"

#include <vector>
#include <memory>
#include <cassert>

namespace vt { namespace auto_registry {

template <typename FunctorT, bool msg, typename... Args>
inline HandlerType makeAutoHandlerFunctor() {
  // Arg (overload) differentiaton in adapter.
  using AdapterType = FunctorAdapterArgs<FunctorT, Args...>;
  using ContainerType = AutoActiveFunctorContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveFunctorType>;
  using FuncType = ActiveFnPtrType;
  using RunType = RunnableFunctor<
    AdapterType, ContainerType, RegInfoType, FuncType, msg
  >;
  return HandlerManagerType::makeHandler(true, true, RunType::idx);
}

template <typename RunnableT, typename RegT, typename InfoT, typename FnT>
RegistrarFunctor<RunnableT, RegT, InfoT, FnT>::RegistrarFunctor() {
  using AdapterType = typename RunnableT::AdapterType;

  RegT& reg = getAutoRegistryGen<RegT>();
  index = reg.size(); // capture current index

  FnT fn = reinterpret_cast<FnT>(AdapterType::getFunction());
  size_t num_args = AdapterType::getNumArgs();

  #if backend_check_enabled(trace_enabled)
  // trace
  std::string event_type_name = AdapterType::traceGetEventType();
  std::string event_name = AdapterType::traceGetEventName();
  auto const& trace_ep = trace::TraceRegistry::registerEventHashed(
    event_type_name, event_name);
  reg.emplace_back(InfoT{NumArgsTag, fn, trace_ep, num_args});
  #else
  // non-trace
  reg.emplace_back(InfoT{NumArgsTag, fn, num_args});
  #endif
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

template <typename RunnableT, typename RegT, typename InfoT, typename FnT>
AutoHandlerType registerActiveFunctor() {
  return RegistrarWrapperFunctor<RunnableT, RegT, InfoT, FnT>().registrar.index;
}

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_FUNCTOR_AUTO_REGISTRY_FUNCTOR_IMPL_H*/
