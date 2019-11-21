/*
//@HEADER
// *****************************************************************************
//
//                         auto_registry_general_impl.h
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

#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_IMPL_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_IMPL_H

#include "vt/config.h"
#include "vt/utils/demangle/demangle.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/objgroup/type_registry/registry.h"

#include <functional>

namespace vt { namespace auto_registry {

template <typename RegObjTypeT>
struct RegistrarGenInfoImpl : RegistrarGenInfoBase {
  virtual HandlerType getRegisteredIndex() override {
    return objgroup::registry::makeObjIdx<RegObjTypeT>();
  }
};

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
RegistrarGen<ActFnT, RegT, InfoT, FnT>::RegistrarGen() {
  RegT& reg = getAutoRegistryGen<RegT>();
  index = reg.size();

  FnT fn = reinterpret_cast<FnT>(ActFnT::getFunction());
  RegistrarGenInfo indexAccessor{new RegistrarGenInfoImpl<typename ActFnT::ObjType>()};

  #if backend_check_enabled(trace_enabled)
  using Tn = typename ActFnT::ActFnType;
  auto const& type_name = util::demangle::DemanglerUtils::getTypeName<Tn>();
  auto const& parsed_type_name =
    util::demangle::ActiveFunctionDemangler::parseActiveFunctionName(type_name);
  auto const& trace_ep = trace::TraceRegistry::registerEventHashed(
    parsed_type_name.getNamespace(), parsed_type_name.getFuncParams()
  );

  reg.emplace_back(InfoT{fn, std::move(indexAccessor), trace_ep});
  #else
  reg.emplace_back(InfoT{fn, std::move(indexAccessor)});
  #endif
}

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
AutoHandlerType registerActiveGen() {
  return RegistrarWrapperGen<ActFnT, RegT, InfoT, FnT>().registrar.index;
}

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
/*static*/ constexpr typename
RunnableGen<ActFnT, RegT, InfoT, FnT>::FunctionPtrType
RunnableGen<ActFnT, RegT, InfoT, FnT>::getFunction() {
  return ActFnT::getFunction();
}

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
AutoHandlerType const RunnableGen<ActFnT, RegT, InfoT, FnT>::idx =
  registerActiveGen<RunnableGen<ActFnT, RegT, InfoT, FnT>, RegT, InfoT, FnT>();

}} /* end namespace vt::auto_registry */

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_IMPL_H*/
