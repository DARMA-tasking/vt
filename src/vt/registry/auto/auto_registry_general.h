/*
//@HEADER
// *****************************************************************************
//
//                           auto_registry_general.h
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
#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"

#include "vt/utils/demangle/demangle.h"

namespace vt { namespace auto_registry {

// Not currently used .. maybe regression?
//template <typename T, typename... Args>
//static inline auto proxyOperatorToNewInstanceRval(Args&&... args) {
//  T instance;
//  return instance.operator()(std::forward<Args>(args)...);
//}

template <typename T, typename... Args>
static inline auto proxyOperatorToNewInstanceReg(Args... args) {
  T instance;
  return instance.operator()(args...);
}

/// All Functor Adapters require:
/// - FunctionPtrType
/// - ObjType
/// - getFunction
/// - traceGetEventType
/// - traceGetEventName
/// (The 'Functor' naming is a bit of a misnomer as they are not functors
///  nor do they strictly wrap functors..)

/// Adapt a type that provides operator()(..)
/// NOTE:
/// MULTIPLE INSTANCES of the type will be created and discarded.
/// This cannot be used for a stateful instance.
/// This is an implementation detail that could be reconsidered.
template <typename ObjTypeT, typename... ArgsT>
struct FunctorAdapterArgs {
  using FunctionPtrType = void (*)(ArgsT...);
  using ObjType = ObjTypeT;

  static constexpr FunctionPtrType getFunction() {
    return &proxyOperatorToNewInstanceReg<ObjType, ArgsT...>;
  }

  static NumArgsType getNumArgs() {
    return sizeof...(ArgsT);
  }

#if vt_check_enabled(trace_enabled)
  static std::string traceGetEventType() {
    using TE = vt::util::demangle::TemplateExtract;
    using DU = vt::util::demangle::DemanglerUtils;
    auto ns = TE::getTypeName<ObjTypeT>();
    if (ns.empty())
      ns = "(none)";
    return DU::removeSpaces(ns);
  }

  static std::string traceGetEventName() {
    using TE = vt::util::demangle::TemplateExtract;
    using DU = vt::util::demangle::DemanglerUtils;
    std::vector<std::string> arg_types = {
      TE::getTypeName<ArgsT>()...
    };
    auto args = DU::join(",", arg_types);
    return DU::removeSpaces("operator(" + args + ")");
  }
#endif // end trace_enabled
};

// Need to provide a non-pointer overload for parameterization auto-registered
// functions for GCC
template <typename F, F f>
struct FunctorAdapterParam {
  using FunctionPtrType = F;
  using ObjType = void;

  static constexpr FunctionPtrType getFunction() { return f; }

  static NumArgsType getNumArgs() {
    return 0; // lies - see NumArgsTag, perhaps
  }

#if vt_check_enabled(trace_enabled)
  static std::string traceGetEventType() {
    using TE = vt::util::demangle::TemplateExtract;
    using DU = vt::util::demangle::DemanglerUtils;
    auto ns = TE::getNamespace(TE::getValueName<F,f>());
    if (ns.empty())
      ns = "(none)";
    return DU::removeSpaces(ns);
  }

  static std::string traceGetEventName() {
    using TE = vt::util::demangle::TemplateExtract;
    using DU = vt::util::demangle::DemanglerUtils;
    auto barename = TE::getBarename(TE::getValueName<F,f>());
    auto args = TE::getVoidFuncStrArgs(TE::getTypeName<F>());
    return DU::removeSpaces(barename + "(" + args + ")");
  }
#endif // end trace_enabled
};

template <typename F, F* f>
struct FunctorAdapter {
  using FunctionPtrType = F*;
  using ObjType = void;

  static constexpr FunctionPtrType getFunction() { return f; }

  static NumArgsType getNumArgs() {
    return 0; // lies - see NumArgsTag, perhaps
  }

#if vt_check_enabled(trace_enabled)
  static std::string traceGetEventType() {
    using TE = vt::util::demangle::TemplateExtract;
    using DU = vt::util::demangle::DemanglerUtils;
    auto ns = TE::getNamespace(TE::getValueNamePtr<F,f>());
    if (ns.empty())
      ns = "(none)";
    return DU::removeSpaces(ns);
  }

  static std::string traceGetEventName() {
    using TE = vt::util::demangle::TemplateExtract;
    using DU = vt::util::demangle::DemanglerUtils;
    auto barename = TE::getBarename(TE::getValueNamePtr<F,f>());
    auto args = TE::getVoidFuncStrArgs(TE::getTypeName<F>());
    return DU::removeSpaces(barename + "(" + args + ")");
  }
#endif // end trace_enabled
};

template <typename F, F f, typename ObjT = void>
struct FunctorAdapterMember {
  using FunctionPtrType = F;
  using ObjType = ObjT;

  static constexpr FunctionPtrType getFunction() { return f; }

  static NumArgsType getNumArgs() {
    return 0; // lies - see NumArgsTag, perhaps
  }

#if vt_check_enabled(trace_enabled)
  static std::string traceGetEventType() {
    using TE = vt::util::demangle::TemplateExtract;
    using DU = vt::util::demangle::DemanglerUtils;
    auto ns = TE::getNamespace(TE::getValueName<F,f>());
    if (ns.empty())
      ns = "(none)";
    return DU::removeSpaces(ns);
  }

  static std::string traceGetEventName() {
    using TE = vt::util::demangle::TemplateExtract;
    using DU = vt::util::demangle::DemanglerUtils;
    auto barename = TE::getBarename(TE::getValueName<F,f>());
    auto args = TE::getVoidFuncStrArgs(TE::getTypeName<F>());
    return DU::removeSpaces(barename + "(" + args + ")");
  }
#endif // end trace_enabled
};

template <typename RegT, typename = void>
RegT& getAutoRegistryGen();

template <typename RegT, typename>
inline RegT& getAutoRegistryGen() {
#pragma sst keep
  static RegT reg;
  return reg;
}

template <typename AdapterT, typename RegT, typename InfoT, typename FnT>
struct RegistrarGen {
  AutoHandlerType index;

  RegistrarGen();
};

template <typename AdapterT, typename RegT, typename InfoT, typename FnT>
struct RegistrarWrapperGen {
  RegistrarGen<AdapterT, RegT, InfoT, FnT> registrar;
};

template <typename AdapterT, typename RegT, typename InfoT, typename FnT>
AutoHandlerType registerActiveGen();

template <typename AdapterT, typename RegT, typename InfoT, typename FnT>
struct RunnableGen {
  using AdapterType = AdapterT;
  using FunctionPtrType = typename AdapterType::FunctionPtrType;
  using ObjType = typename AdapterType::ObjType;

  static AutoHandlerType const idx;
  static constexpr FunctionPtrType getFunction();

  RunnableGen() = default;
};

}} // end namespace vt::auto_registry

#include "vt/registry/auto/auto_registry_general_impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_H*/
