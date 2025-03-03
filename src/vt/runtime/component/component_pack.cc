/*
//@HEADER
// *****************************************************************************
//
//                              component_pack.cc
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_CC
#define INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_CC

#include "vt/runtime/component/component_pack.h"

namespace vt { namespace runtime { namespace component {

void ComponentPack::construct() {
  addAllRequiredComponents();

  // Topologically sort the components based on dependencies to generate a
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

void ComponentPack::foreach(std::function<void(BaseComponent*)> apply) {
  for (auto&& elm : live_components_) {
    apply(elm.get());
  }
}

void ComponentPack::destruct() {
  live_ = false;
  pollable_components_.clear();
  while (live_components_.size() > 0) {
    if (live_components_.back() != nullptr) {
      vt_debug_print(
        normal, runtime,
        "ComponentPack: finalizing component={}\n",
        live_components_.back()->name()
      );

      live_components_.back()->finalize();
    }
    live_components_.pop_back();
  }
}

int ComponentPack::progress(TimeType current_time) {
  int total = 0;
  for (auto&& pollable : pollable_components_) {
    total += pollable->progress(current_time);
  }
  return total;
}

bool ComponentPack::needsCurrentTime() {
  bool needs_time = false;
  for (auto&& pollable : pollable_components_) {
    needs_time = needs_time || pollable->needsCurrentTime();
  }
  return needs_time;
}

void ComponentPack::addAllRequiredComponents() {
  bool inserted = false;
  do {
    inserted = false;
    for (auto&& i : added_components_) {
      auto s_deps = registry::getStartupDeps(i);
      auto r_deps = registry::getRuntimeDeps(i);
      for (auto&& sd : s_deps) {
        if (added_components_.find(sd) == added_components_.end()) {
          added_components_.insert(sd);
          inserted = true;
        }
      }
      for (auto&& rd : r_deps) {
        if (added_components_.find(rd) == added_components_.end()) {
          added_components_.insert(rd);
          inserted = true;
        }
      }
    }
  } while (inserted);
}

std::list<int> ComponentPack::topoSort() {
  std::list<int> order;

  registry::AutoHandlerType max_idx = 0;
  for (auto&& r : registered_components_) {
    max_idx = std::max(r, max_idx);
  }

  auto visited = std::make_unique<bool[]>(max_idx + 1);
  auto visiting = std::make_unique<bool[]>(max_idx + 1);
  //fmt::print("added size={}\n", added_components_.size());
  for (auto&& i : added_components_) {
    if (visited[i] == false) {
      topoSortImpl(i, order, visited.get(), visiting.get());
    }
  }

  return order;
}

void ComponentPack::topoSortImpl(
  int v, std::list<int>& order, bool* visited, bool* visiting
) {
  //fmt::print("topoSortImpl v={}\n",v);

  vtAbortIf(visiting[v] == true, "Already visiting this node, cycle detected");
  visiting[v] = true;

  auto v_list = registry::getStartupDeps(v);
  for (auto&& vp : v_list) {
    //fmt::print("v={} vp={}, size={}\n",v,vp,v_list.size());

    if (not visited[vp]) {
      topoSortImpl(vp, order, visited, visiting);
    }
  }

  visiting[v] = false;
  visited[v] = true;
  order.push_front(v);
}

}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_CC*/
