/*
//@HEADER
// *****************************************************************************
//
//                               component_pack.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_H
#define INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_H

#include "vt/config.h"
#include "vt/runtime/component/component.h"

#include <list>
#include <unordered_map>
#include <unordered_set>

namespace vt { namespace runtime { namespace component {

struct ComponentPack {

  template <typename T, typename... Deps, typename... Cons>
  registry::AutoHandlerType registerComponent(
    T** ref, typename BaseComponent::DepsPack<Deps...>, Cons&&... cons
  ) {
    ComponentRegistry::dependsOn<T, Deps...>();
    auto idx = registry::makeIdx<T>();
    //fmt::print("register: i={} size={}\n", idx, sizeof...(Deps));
    registered_components_.push_back(idx);
    construct_components_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(idx),
      std::forward_as_tuple(
        //[this, args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        [ref,this,cons...]() mutable {
          auto ptr = T::template staticInit<Cons...>(std::forward<Cons>(cons)...);
          // auto ptr = make<T, std::tuple<Args...>>(std::forward<std::tuple<Args...>>(args));

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
      )
    );
    return idx;
  }

// private:
//   template <typename T, typename Tuple, typename... Args>
//   std::unique_ptr<T> make(std::tuple<Args...>&& tup) {
//     static constexpr auto size = std::tuple_size<Tuple>::value;
//     static constexpr auto seq = std::make_index_sequence<size>{};
//     return makeImpl<T, Tuple>(std::forward<std::tuple<Args...>>(tup), seq);
//   }

//   template <typename T, typename Tuple, typename... Args, size_t... I>
//   std::unique_ptr<T> makeImpl(std::tuple<Args...>&& tup, std::index_sequence<I...> seq) {
//     return T::template staticInit<Args...>(
//       std::forward<typename std::tuple_element<I,Tuple>::type>(
//         std::get<I>(tup)
//       )...
//     );
//   }

public:

  template <typename T>
  void add() {
    auto idx = registry::makeIdx<T>();
    added_components_.insert(idx);
  }

  void construct() {
    // Topologically sort the components based on dependences to generate a
    // valid startup order
    auto order = topoSort();

    // Construct the components, and fire off initialize
    while (not order.empty()) {
      auto next = order.back();
      fmt::print("constructing = {}\n", next);
      order.pop_back();

      auto iter = construct_components_.find(next);
      if (iter != construct_components_.end()) {
        iter->second();
      } else {
        vtAbort("Could not find component to construct");
      }
    }

    // Run the startup now that all components are live
    for (auto&& c : live_components_) {
      c->startup();
    }

    live_ = true;
  }

  ~ComponentPack() {
    destruct();
  }

  void destruct() {
    live_ = false;
    pollable_components_.clear();
    while (live_components_.size() > 0) {
      live_components_.back()->finalize();
      live_components_.pop_back();
    }
  }

private:
  std::list<int> topoSort() {
    std::list<int> order;
    fmt::print("added size={}\n", added_components_.size());
    auto visited = std::make_unique<bool[]>(registered_components_.size());
    for (std::size_t i = 0; i < registered_components_.size(); i++) {
      auto added_iter = added_components_.find(i);
      if (added_iter != added_components_.end()) {
        if (visited[i] == false) {
          topoSortImpl(i, order, visited.get());
        }
      }
    }

    detectCycles(order.front());

    return order;
  }

  void topoSortImpl(int v, std::list<int>& order, bool* visited) {
    fmt::print("impl v={}\n",v);
    visited[v] = true;

    auto v_list = registry::getIdx(v);
    for (auto&& vp : v_list) {
      fmt::print("v={} vp={}, size={}\n",v,vp,v_list.size());
      auto iter = added_components_.find(vp);
      if (iter == added_components_.end()) {
        fmt::print("Adding component automatically due to dependencies\n");
      }
      if (not visited[vp]) {
        topoSortImpl(vp, order, visited);
      }
    }

    order.push_front(v);
  }

  void detectCycles(int root) {
    std::list<int> stack;
    detectCyclesImpl(stack, root);
  }

  void detectCyclesImpl(std::list<int>& stack, int dep) {
    for (auto&& prev_dep : stack) {
      if (prev_dep == dep) {
        vtAbort("Cycle found in dependencies of registered components\n");
      }
    }

    stack.push_back(dep);
    auto vl = registry::getIdx(dep);
    for (auto&& vp : vl) {
      detectCyclesImpl(stack, vp);
    }
    stack.pop_back();
  }

public:
  bool live() const { return live_; }
  bool registrationsDone() const { return registrations_done_; }
  void finishRegistration() { registrations_done_ = true; }

private:
  bool registrations_done_ = false;
  bool live_ = false;
  std::vector<registry::AutoHandlerType> registered_components_;
  std::unordered_set<registry::AutoHandlerType> added_components_;
  std::unordered_map<registry::AutoHandlerType, ActionType> construct_components_;
  std::vector<std::unique_ptr<BaseComponent>> live_components_;
  std::vector<Progressable*> pollable_components_;
};

template <typename... Ts>
using Deps = typename BaseComponent::DepsPack<Ts...>;

}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_H*/
