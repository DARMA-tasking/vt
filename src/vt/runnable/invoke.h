/*
//@HEADER
// *****************************************************************************
//
//                                   invoke.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_RUNNABLE_INVOKE_H
#define INCLUDED_VT_RUNNABLE_INVOKE_H

#include "vt/config.h"

#if vt_check_enabled(trace_enabled)
#include "vt/utils/demangle/demangle.h"
#include "vt/utils/static_checks/function_ret_check.h"
#include "vt/trace/trace_registry.h"

#include <type_traits>
#include <functional>
#endif

#include <utility>

namespace vt { namespace runnable {

#if vt_check_enabled(trace_enabled)

template <typename FunctionType, FunctionType f>
static std::string CreateEventTypeCStyleFunc() {
  using TE = vt::util::demangle::TemplateExtract;
  using DU = vt::util::demangle::DemanglerUtils;

  auto ns = TE::getNamespace(TE::getValueName<std::decay_t<FunctionType>, f>());
  if (ns.empty())
    ns = "(none)";

  return DU::removeSpaces(ns);
}

template <typename Class>
static std::string CreateEventTypeMemberFunc() {
  using TE = vt::util::demangle::TemplateExtract;
  using DU = vt::util::demangle::DemanglerUtils;

  auto typeName = TE::getTypeName<std::decay_t<Class>>();
  if (typeName.empty())
    typeName = "(none)";

  return DU::removeSpaces(typeName);
}

template <typename FunctionType, FunctionType f, typename... Args>
static std::string CreateEventName() {
  using TE = vt::util::demangle::TemplateExtract;
  using DU = vt::util::demangle::DemanglerUtils;

  std::vector<std::string> arg_types = {TE::getTypeName<Args>()...};
  auto argsV = DU::join(",", arg_types);
  auto valueName = TE::getValueName<FunctionType, f>();
  auto barename = TE::getBarename(valueName);

  return DU::removeSpaces(barename + "(" + argsV + ")");
}

template <auto f>
struct CallableWrapper {
  using Type = decltype(f);

  static std::string GetEventTypeName() {
    if constexpr (std::is_member_function_pointer_v<Type>) {
      return CreateEventTypeMemberFunc<util::FunctionWrapper<Type>>();
    } else {
      return CreateEventTypeCStyleFunc<Type, f>();
    }
  }

  static std::string GetEventName() {
    return CreateEventName<Type, f>();
  }

  static trace::TraceEntryIDType GetTraceID() {
    return trace::TraceRegistry::registerEventHashed(
      GetEventTypeName(), GetEventName());
  }
};

template <auto f, typename... Args>
struct ScopedInvokeEvent {
  ScopedInvokeEvent() {
    const auto trace_id = CallableWrapper<f>::GetTraceID();
    const auto trace_event = theTrace()->messageCreation(trace_id, 0);
    const auto from_node = theContext()->getNode();

    tag_ = theTrace()->beginProcessing(
      trace_id, 0, trace_event, from_node, timing::getCurrentTime()
    );
  }

  ~ScopedInvokeEvent() {
    theTrace()->endProcessing(tag_, timing::getCurrentTime());
    theTrace()->messageCreation(CallableWrapper<f>::GetTraceID(), 0);
  }

private:
  trace::TraceProcessingTag tag_ = {};
};

#endif // vt_check_enabled(trace_enabled)

template <auto f, typename... Args>
auto invoke(Args&&... args){
#if vt_check_enabled(trace_enabled)
    ScopedInvokeEvent<f> e;
#endif

  return std::invoke(std::forward<decltype(f)>(f), std::forward<Args>(args)...);
}

template <typename Callable, Callable f, typename... Args>
auto invoke(Args&&... args) {
  return invoke<f>(std::forward<Args>(args)...);
}

}} // namespace vt::runnable

#endif /*INCLUDED_VT_RUNNABLE_INVOKE_H*/
