/*
//@HEADER
// *****************************************************************************
//
//                             component_registry.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_REGISTRY_H
#define INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_REGISTRY_H

#include "vt/configs/types/types_type.h"

#include <vector>
#include <tuple>
#include <unordered_set>

namespace vt { namespace runtime { namespace component { namespace registry {

using AutoHandlerType = HandlerType;

struct RegisteredDeps {

  RegisteredDeps() = default;

  void addStartupDep(AutoHandlerType dep) {
    startup_deps_.insert(dep);
  }

  void addRuntimeDep(AutoHandlerType dep) {
    runtime_deps_.insert(dep);
  }

  auto const& getStartupDeps() const { return startup_deps_; }
  auto const& getRuntimeDeps() const { return runtime_deps_; }

private:
  std::unordered_set<AutoHandlerType> startup_deps_; /**< Startup dependencies */
  std::unordered_set<AutoHandlerType> runtime_deps_; /**< Runtime dependencies */
};

using RegistryType = std::vector<RegisteredDeps>;

inline RegistryType& getRegistry() {
  static RegistryType reg;
  return reg;
}

template <typename ObjT>
struct Registrar {
  Registrar() {
    auto& reg = getRegistry();
    index = reg.size();
    reg.emplace_back(RegisteredDeps{});
  }
  AutoHandlerType index;
};

template <typename ObjT>
struct Type {
  static AutoHandlerType const idx;
};

template <typename ObjT>
AutoHandlerType const Type<ObjT>::idx = Registrar<ObjT>().index;

inline auto& getIdx(AutoHandlerType han) {
  return getRegistry().at(han);
}

inline auto getStartupDeps(AutoHandlerType han) {
  return getRegistry().at(han).getStartupDeps();
}

inline auto getRuntimeDeps(AutoHandlerType han) {
  return getRegistry().at(han).getStartupDeps();
}

template <typename ObjT>
inline AutoHandlerType makeIdx() {
  return Type<ObjT>::idx;
}

}}}} /* end namespace vt::runtime::component::registry */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_REGISTRY_H*/
