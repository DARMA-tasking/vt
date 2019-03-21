/*
//@HEADER
// ************************************************************************
//
//                          auto_registry_general.h
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
#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"

namespace vt { namespace auto_registry {

template <typename F, F* f>
struct FunctorAdapter {
  using FunctionPtrType = F*;
  using ObjType = void;

  static constexpr FunctionPtrType getFunction() { return f; }

  template <typename... A>
  auto operator()(A&&... a) -> decltype(auto) {
    return f(std::forward<A>(a)...);
   }
};

template <typename F, F f, typename ObjT = void>
struct FunctorAdapterMember {
  using FunctionPtrType = F;
  using ObjType = ObjT;

  static constexpr FunctionPtrType getFunction() { return f; }

  template <typename... A>
  auto operator()(A&&... a) -> decltype(auto) {
    return f(std::forward<A>(a)...);
   }
};

template <typename RegistryT, typename = void>
RegistryT& getAutoRegistryGen();

template <typename RegistryT, typename>
inline RegistryT& getAutoRegistryGen() {
#pragma sst keep
  static RegistryT reg;
  return reg;
}

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
struct RegistrarGen {
  AutoHandlerType index;

  RegistrarGen();
};

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
struct RegistrarWrapperGen {
  RegistrarGen<ActFnT, RegT, InfoT, FnT> registrar;
};

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
AutoHandlerType registerActiveGen();

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
struct RunnableGen {
  using ActFnType = ActFnT;
  using FunctionPtrType = typename ActFnT::FunctionPtrType;
  using ObjType = typename ActFnT::ObjType;

  static AutoHandlerType const idx;
  static constexpr FunctionPtrType getFunction();

  RunnableGen() = default;
};

}} // end namespace vt::auto_registry

#include "vt/registry/auto/auto_registry_general_impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_H*/
