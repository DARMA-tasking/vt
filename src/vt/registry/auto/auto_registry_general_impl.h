/*
//@HEADER
// ************************************************************************
//
//                          auto_registry_general_impl.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
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

#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_IMPL_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_IMPL_H

#include "vt/config.h"
#include "vt/utils/demangle/demangle.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"

namespace vt { namespace auto_registry {

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
RegistrarGen<ActFnT, RegT, InfoT, FnT>::RegistrarGen() {
  RegT& reg = getAutoRegistryGen<RegT>();
  index = reg.size();

  auto fn = ActFnT::getFunction();

  #if backend_check_enabled(trace_enabled)
  using Tn = typename ActFnT::ActFnType;
  auto const& type_name = util::demangle::DemanglerUtils::getTypeName<Tn>();
  auto const& parsed_type_name =
    util::demangle::ActiveFunctionDemangler::parseActiveFunctionName(type_name);
  auto const& trace_ep = trace::TraceRegistry::registerEventHashed(
    parsed_type_name.getNamespace(), parsed_type_name.getFuncParams()
  );

  reg.emplace_back(InfoT{reinterpret_cast<FnT>(fn), trace_ep});
  #else
  reg.emplace_back(InfoT{reinterpret_cast<FnT>(fn)});
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
