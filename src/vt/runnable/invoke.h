/*
//@HEADER
// *****************************************************************************
//
//                                   invoke.h
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

#if !defined INCLUDED_VT_RUNNABLE_INVOKE_H
#define INCLUDED_VT_RUNNABLE_INVOKE_H

#include "vt/config.h"
#include "vt/utils/demangle/demangle.h"
#include "vt/utils/static_checks/function_ret_check.h"
#include "vt/trace/trace_registry.h"

#include <type_traits>

namespace vt { namespace runnable {

template <typename FunctionType, FunctionType f>
static std::string CreatetEventTypeCStyleFunc() {
  using TE = vt::util::demangle::TemplateExtract;
  using DU = vt::util::demangle::DemanglerUtils;

  auto ns = TE::getNamespace(TE::getValueName<std::decay_t<FunctionType>, f>());
  if (ns.empty())
    ns = "(none)";

  return DU::removeSpaces(ns);
}

template <typename Class>
static std::string CreatetEventTypeMemberFunc() {
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

template <typename FunctionType, FunctionType f>
struct CallableWrapper;

template <typename Ret, typename... Args, Ret (*f)(Args...)>
struct CallableWrapper<Ret(*)(Args...), f> {
  using Type = Ret(*)(Args...);

  static std::string GetEventTypeName() {
    return CreatetEventTypeCStyleFunc<Type, f>();
  }

  static std::string GetEventName() {
    return CreateEventName<Type, f, Args...>();
  }

  static trace::TraceEntryIDType GetTraceID() {
    return trace::TraceRegistry::registerEventHashed(
      GetEventTypeName(), GetEventName()
    );
  }
};

template <
  typename Ret, typename Class, typename... Args, Ret (Class::*f)(Args...)
>
struct CallableWrapper<Ret (Class::*)(Args...), f> {
  using Type = Ret (Class::*)(Args...);

  static std::string GetEventTypeName() {
    return CreatetEventTypeMemberFunc<Class>();
  }

  static std::string GetEventName() {
    return CreateEventName<Type, f, Args...>();
  }

  static trace::TraceEntryIDType GetTraceID() {
    return trace::TraceRegistry::registerEventHashed(
      GetEventTypeName(), GetEventName()
    );
  }
};

#if vt_check_enabled(trace_enabled)
template <typename Callable, Callable f, typename... Args>
static trace::TraceProcessingTag BeginProcessingInvokeEvent() {
  const auto trace_id = CallableWrapper<Callable, f>::GetTraceID();
  const auto trace_event = theTrace()->messageCreation(trace_id, 0);
  const auto from_node = theContext()->getNode();

  return theTrace()->beginProcessing(trace_id, 0, trace_event, from_node, timing::getCurrentTime());
}

template <typename Callable, Callable f, typename... Args>
static void EndProcessingInvokeEvent(trace::TraceProcessingTag processing_tag) {
  theTrace()->endProcessing(processing_tag, timing::getCurrentTime());

  const auto trace_id = CallableWrapper<Callable, f>::GetTraceID();
  theTrace()->messageCreation(trace_id, 0);
}
#endif

template <
  typename Fn, typename Type, typename T1,
  typename std::enable_if_t<std::is_pointer<std::decay_t<T1>>::value, int> = 0,
  typename... Args
>
decltype(auto) invokeImpl(Type Fn::*f, T1&& obj, Args&&... args) {
  return ((*std::forward<T1>(obj)).*f)(std::forward<Args>(args)...);
}

template <
  typename Fn, typename Type, typename T1,
  typename std::enable_if_t<!std::is_pointer<std::decay_t<T1>>::value, int> = 0,
  typename... Args
>
decltype(auto) invokeImpl(Type Fn::*f, T1&& obj, Args&&... args) {
  return (std::forward<T1>(obj).*f)(std::forward<Args>(args)...);
}

template <typename Callable, typename... Args>
decltype(auto) invokeImpl(Callable&& f, Args&&... args) {
  return std::forward<Callable>(f)(std::forward<Args>(args)...);
}

template <typename Callable, Callable f, typename... Args>
util::Copyable<Callable> invoke(Args&&... args) {
#if vt_check_enabled(trace_enabled)
  const auto processing_tag =
    BeginProcessingInvokeEvent<Callable, f>();
#endif

  const auto& returnVal = invokeImpl(f, std::forward<Args>(args)...);

#if vt_check_enabled(trace_enabled)
  EndProcessingInvokeEvent<Callable, f>(processing_tag);
#endif

  return returnVal;
}

template <typename Callable, Callable f, typename... Args>
util::NotCopyable<Callable> invoke(Args&&... args) {
#if vt_check_enabled(trace_enabled)
  const auto processing_tag =
    BeginProcessingInvokeEvent<Callable, f>();
#endif

  auto&& returnVal = invokeImpl(f, std::forward<Args>(args)...);

#if vt_check_enabled(trace_enabled)
  EndProcessingInvokeEvent<Callable, f>(processing_tag);
#endif

  return std::move(returnVal);
}

template <typename Callable, Callable f, typename... Args>
util::IsVoidReturn<Callable> invoke(Args&&... args) {
#if vt_check_enabled(trace_enabled)
  const auto processing_tag =
    BeginProcessingInvokeEvent<Callable, f>();
#endif

  invokeImpl(f, std::forward<Args>(args)...);

#if vt_check_enabled(trace_enabled)
  EndProcessingInvokeEvent<Callable, f>(processing_tag);
#endif
}

}} // namespace vt::runnable

#endif /*INCLUDED_VT_RUNNABLE_INVOKE_H*/
