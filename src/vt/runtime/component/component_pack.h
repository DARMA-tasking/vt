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

#include "vt/runtime/component/component.h"

#include <list>
#include <unordered_map>
#include <unordered_set>

namespace vt { namespace runtime { namespace component {

struct ComponentPack {

  ComponentPack() = default;

public:
  template <typename T, typename... Deps, typename... Cons>
  registry::AutoHandlerType registerComponent(
    T** ref, typename BaseComponent::DepsPack<Deps...>, Cons&&... cons
  );

  template <typename T>
  void add();

  void construct();

  ~ComponentPack();

  void destruct();

  int progress();

private:
  std::list<int> topoSort();

  void topoSortImpl(int v, std::list<int>& order, bool* visited);

  void detectCycles(int root);

  void detectCyclesImpl(std::list<int>& stack, int dep);

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

#include "vt/runtime/component/component_pack.impl.h"

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_H*/
