/*
//@HEADER
// *****************************************************************************
//
//                                 invoke.h
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

#if !defined INCLUDED_RUNNABLE_INVOKE_H
#define INCLUDED_RUNNABLE_INVOKE_H

#include "vt/config.h"
#include "vt/utils/demangle/demangle.h"

#include <type_traits>

namespace vt { namespace runnable {

template <typename FunctionType, FunctionType f>
struct CallableWrapper;

template <typename FunctionType, FunctionType f>
constexpr decltype(auto) CreatetEventTypeCStyleFunc() {
  using TE = vt::util::demangle::TemplateExtract;
  using DU = vt::util::demangle::DemanglerUtils;

  auto ns = TE::getNamespace(TE::getValueName<FunctionType, f>());
  if (ns.empty())
    ns = "(none)";
  return DU::removeSpaces(ns);
}

template <typename Class>
constexpr decltype(auto) CreatetEventTypeMemberFunc() {
  using TE = vt::util::demangle::TemplateExtract;
  using DU = vt::util::demangle::DemanglerUtils;

  auto typeName = TE::getTypeName<Class>();
  if (typeName.empty())
    typeName = "(none)";
  return DU::removeSpaces(typeName);
}

template <typename FunctionType, FunctionType f, typename... Args>
constexpr decltype(auto) CreateEventName() {
  using TE = vt::util::demangle::TemplateExtract;
  using DU = vt::util::demangle::DemanglerUtils;

  std::vector<std::string> arg_types = {TE::getTypeName<Args>()...};
  auto argsV = DU::join(",", arg_types);
  auto valueName = TE::getValueName<FunctionType, f>();
  auto barename = TE::getBarename(valueName);
  return DU::removeSpaces(barename + "(" + argsV + ")");
}

template <typename Ret, typename... Args, Ret (*f)(Args...)>
struct CallableWrapper<Ret(*)(Args...), f> {
  using ReturnType = Ret;
  using Type = Ret(*)(Args...);

  static decltype(auto) GetTraceID() {
    return trace::TraceRegistry::registerEventHashed(
      CreatetEventTypeCStyleFunc<Type, f>(),
      CreateEventName<Type, f>());
  }
};

template <
  typename Ret, typename Class, typename... Args, Ret (Class::*f)(Args...)
>
struct CallableWrapper<Ret (Class::*)(Args...), f> {
  using ReturnType = Ret;
  using Type = Ret (Class::*)(Args...);

  static decltype(auto) GetTraceID() {
    return trace::TraceRegistry::registerEventHashed(
      CreatetEventTypeMemberFunc<Class>(), CreateEventName<Type, f>()
    );
  }
};

template <typename FunctionType>
using IsVoidReturn = std::enable_if_t<
  std::is_same<
    typename CallableWrapper<FunctionType, nullptr>::ReturnType, void>::value,
  void
>;

template <
  typename FunctionType,
  typename Ret = typename CallableWrapper<FunctionType, nullptr>::ReturnType
>
using Copyable = std::enable_if_t<
  !std::is_same<Ret, void>::value && std::is_copy_constructible<Ret>::value,
  Ret
>;

template <
  typename FunctionType,
  typename Ret = typename CallableWrapper<FunctionType, nullptr>::ReturnType
>
using NotCopyable = std::enable_if_t<
  !std::is_same<Ret, void>::value && !std::is_copy_constructible<Ret>::value,
  Ret
>;

template <typename Callable, Callable f, typename... Args>
constexpr decltype(auto) BeginProcessingInvokeEvent() {
  const auto trace_id = CallableWrapper<Callable, f>::GetTraceID();
  const auto trace_event = theTrace()->localInvoke(trace_id);
  const auto from_node = theContext()->getNode();

  return theTrace()->beginProcessing(trace_id, 0, trace_event, from_node);
}

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

template <typename RetT, typename... Args>
decltype(auto) invokeImpl(RetT (*f)(Args...), Args&&... args) {
  return (*f)(std::forward<Args>(args)...);
}

template <typename Callable, Callable f, typename... Args>
constexpr inline Copyable<Callable> invoke(Args&&... args) {
#if vt_check_enabled(trace_enabled)
  const auto processing_tag =
    BeginProcessingInvokeEvent<Callable, f>();
#endif

  const auto& returnVal = invokeImpl(f, std::forward<Args>(args)...);

#if vt_check_enabled(trace_enabled)
  theTrace()->endProcessing(processing_tag);
#endif

  return returnVal;
}

template <typename Callable, Callable f, typename... Args>
constexpr inline NotCopyable<Callable> invoke(Args&&... args) {
#if vt_check_enabled(trace_enabled)
  const auto processing_tag =
    BeginProcessingInvokeEvent<Callable, f>();
#endif

  auto&& returnVal = invokeImpl(f, std::forward<Args>(args)...);

#if vt_check_enabled(trace_enabled)
  theTrace()->endProcessing(processing_tag);
#endif

  return std::move(returnVal);
}

template <typename Callable, Callable f, typename... Args>
constexpr inline IsVoidReturn<Callable> invoke(Args&&... args) {
#if vt_check_enabled(trace_enabled)
  const auto processing_tag =
    BeginProcessingInvokeEvent<Callable, f>();
#endif

  invokeImpl(f, std::forward<Args>(args)...);

#if vt_check_enabled(trace_enabled)
  theTrace()->endProcessing(processing_tag);
#endif
}

}} // namespace vt::runnable

#endif /*INCLUDED_RUNNABLE_INVOKE_H*/