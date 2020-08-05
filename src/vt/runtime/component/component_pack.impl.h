/*
//@HEADER
// *****************************************************************************
//
//                            component_pack.impl.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_IMPL_H
#define INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_IMPL_H

#include "vt/runtime/component/component_pack.h"

namespace vt { namespace runtime { namespace component {

namespace {

template <typename T, typename Tuple, size_t... I>
std::unique_ptr<T> tupleConsImpl(Tuple&& tup, std::index_sequence<I...> seq) {
  return T::template staticInit(std::get<I>(std::forward<Tuple>(tup))...);
}

template <typename T, typename Tuple>
std::unique_ptr<T> tupleCons(Tuple&& tup) {
  static constexpr auto size = std::tuple_size<std::decay_t<Tuple>>::value;
  using Indices = std::make_index_sequence<size>;
  return tupleConsImpl<T>(std::forward<Tuple>(tup), Indices());
}

template <typename Callable>
std::unique_ptr<MovableFn> makeCallable(Callable&& c) {
  return std::make_unique<MovableFnTyped<Callable>>(std::move(c));
}

} /* end anon namespace */

template <typename T, typename... Deps, typename... Cons>
registry::AutoHandlerType ComponentPack::registerComponent(
  T** ref, typename BaseComponent::DepsPack<Deps...>, Cons&&... cons
) {
  ComponentRegistry::dependsOn<T, Deps...>();
  auto idx = registry::makeIdx<T>();

  if (registered_set_.find(idx) == registered_set_.end()) {
    registered_set_.insert(idx);
    registered_components_.push_back(idx);
  }

  // Be very careful here. Don't use std::make_tuple or we loose l-val refs
  // (which initially was a "feature", but I realized later was probably not
  // ideal)
  std::tuple<Cons...> cons_tuple{std::forward<Cons>(cons)...};

  auto fn = makeCallable(
    [ref, this, tup = std::move(cons_tuple)]() mutable {
      auto ptr = tupleCons<T>(std::move(tup));

      vt_debug_print(
        runtime, node,
        "ComponentPack: constructed component={}, pollable={}\n",
        ptr->name(), ptr->pollable()
      );

      // Set the reference for access outside
      if (ref != nullptr) {
        *ref = ptr.get();
      }

      // Add to pollable for the progress function to be triggered
      if (ptr->pollable()) {
        pollable_components_.emplace_back(ptr.get());
      }

      ptr->initialize();
      live_components_.emplace_back(std::move(ptr));
    }
  );

  construct_components_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(idx),
    std::forward_as_tuple(std::move(fn))
  );
  return idx;
}

template <typename T>
void ComponentPack::add() {
  auto idx = registry::makeIdx<T>();
  added_components_.insert(idx);
}

}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_IMPL_H*/
