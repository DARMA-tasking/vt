/*
//@HEADER
// *****************************************************************************
//
//                              component_pack.cc
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_CC
#define INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_CC

#include "vt/runtime/component/component_pack.h"

namespace vt { namespace runtime { namespace component {

void ComponentPack::construct() {
  // Topologically sort the components based on dependences to generate a
  // valid startup order
  auto order = topoSort();

  // Construct the components, and fire off initialize
  while (not order.empty()) {
    auto next = order.back();
    order.pop_back();

    auto iter = construct_components_.find(next);
    if (iter != construct_components_.end()) {
      iter->second->invoke();
    } else {
      vtAbort("Could not find component to construct");
    }
  }

  // Run the startup now that all components are live
  for (auto&& c : live_components_) {
    c->component_id_ = cur_id_++;
    c->startup();
  }

  live_ = true;
}

ComponentPack::~ComponentPack() {
  destruct();
}

void ComponentPack::destruct() {
  live_ = false;
  pollable_components_.clear();
  while (live_components_.size() > 0) {
    vt_debug_print(
      runtime, node,
      "ComponentPack: finalizing component={}\n",
      live_components_.back()->name()
    );

    live_components_.back()->finalize();
    live_components_.pop_back();
  }
}

int ComponentPack::progress() {
  int total = 0;
  for (auto&& pollable : pollable_components_) {
    total += pollable->progress();
  }
  return total;
}

std::list<int> ComponentPack::topoSort() {
  std::list<int> order;
  //fmt::print("added size={}\n", added_components_.size());
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

void ComponentPack::topoSortImpl(int v, std::list<int>& order, bool* visited) {
  //fmt::print("impl v={}\n",v);
  visited[v] = true;

  auto v_list = registry::getIdx(v);
  for (auto&& vp : v_list) {
    //fmt::print("v={} vp={}, size={}\n",v,vp,v_list.size());
    auto iter = added_components_.find(vp);
    if (iter == added_components_.end()) {
      //fmt::print("Adding component automatically due to dependencies\n");
    }
    if (not visited[vp]) {
      topoSortImpl(vp, order, visited);
    }
  }

  order.push_front(v);
}

void ComponentPack::detectCycles(int root) {
  std::list<int> stack;
  detectCyclesImpl(stack, root);
}

void ComponentPack::detectCyclesImpl(std::list<int>& stack, int dep) {
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

}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_CC*/
