/*
//@HEADER
// *****************************************************************************
//
//                               component_pack.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_H
#define INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_H

#include "vt/runtime/component/component.h"
#include "vt/runtime/component/movable_fn.h"

#include <list>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>

namespace vt { namespace runtime { namespace component {

/**
 * \struct ComponentPack component_pack.h vt/runtime/component/component_pack.h
 *
 * \brief \c ComponentPack class for holding a set of runtime components that
 * make up a coherent inter-dependent runtime
 */
struct ComponentPack {
  using Callable = std::unique_ptr<MovableFn>;

  ComponentPack() = default;

public:
  /**
   * \internal \brief Idempotent registration of a component into this runtime
   * pack. Component dependencies specified with variadic template parameters in
   * \c DepsPack. Registration does not imply the component will be created; it
   * must be added subsequently to be enabled. It simply declares its existence
   * and connectivity with other components.
   *
   * \param[out] ref dumb pointer for access outside
   * \param[in] cons constructor arguments for the component---bound at
   * registration time
   *
   * \return \c registry::AutoHandlerType with type ID for component
   */
  template <typename T, typename... Deps, typename... Cons>
  registry::AutoHandlerType registerComponent(
    T** ref, typename BaseComponent::DepsPack<Deps...>, Cons&&... cons
  );

  /**
   * \internal \brief Add a component to the pack. It will be constructed along
   * with all its dependencies. It must be registered via \c registerComponent
   * before adding.
   */
  template <typename T>
  void add();

  /**
   * \internal \brief Construct all added components along with dependences
   * transitively.
   */
  void construct();

  ~ComponentPack();

  /**
   * \internal \brief Destruct all live components. Can be re-constructed by
   * invoking \c construct
   */
  void destruct();

  /**
   * \internal \brief Invoke the progress function on all pollable components
   *
   * \return the number of work units processed
   */
  int progress();

  /**
   * \internal \brief Extract the first component from a running pack that
   * matches \c name.
   *
   * The component will be cast to the specified component type \c T.
   *
   * \param[in] name the name of the component to remove
   *
   * \return pointer to the component to extract
   */
  template <typename T>
  std::unique_ptr<T> extractComponent(std::string const& name);

  /**
   * \internal \brief Apply a function to each live component in the pack
   *
   * \param[in] apply function that takes a \c BaseComponent to apply over pack
   */
  void foreach(std::function<void(BaseComponent*)> apply);

private:
  /**
   * \internal \brief Topologically sort all the registered components to find a
   * valid activation order based on registered dependences.
   *
   * \return list of handlers representing a valid total ordering that does not
   * break dependences
   */
  std::list<int> topoSort();

  /**
   * \internal \brief Topologically sort all the registered components to find a
   * valid activation order based on registered dependences.
   *
   * \param[in] v current vertex
   * \param[in] order topological order derived so far
   * \param[in] visited array of visited vertices
   */
  void topoSortImpl(int v, std::list<int>& order, bool* visited);

  /**
   * \internal \brief Detect cycles in the dependence graph
   *
   * \param[in] root starting vertex
   */
  void detectCycles(int root);

  /**
   * \internal \brief Detect cycles in the dependence graph
   *
   * \param[in] stack current stack of traversed vertices
   * \param[in] dep next vertex to consider
   */
  void detectCyclesImpl(std::list<int>& stack, int dep);

public:

  /**
   * \internal \brief Query whether the \c ComponentPack is live
   *
   * \return whether it is live
   */
  bool isLive() const { return live_; }

private:
  /// Whether the pack is live
  bool live_ = false;
  /// List of registered components
  std::vector<registry::AutoHandlerType> registered_components_;
  /// Set of registered components to make it idempotent
  std::unordered_set<registry::AutoHandlerType> registered_set_;
  /// Set of added components to be constructed
  std::unordered_set<registry::AutoHandlerType> added_components_;
  /// Bound constructors for components
  std::unordered_map<registry::AutoHandlerType, Callable> construct_components_;
  /// Set of owning pointers to live components
  std::vector<std::unique_ptr<BaseComponent>> live_components_;
  /// Set of non-owning pointers to pollable components for progress engine
  std::vector<Progressable*> pollable_components_;
  /// Component ID for assigning during construction
  ComponentIDType cur_id_ = 1;
};

template <typename... Ts>
using Deps = typename BaseComponent::DepsPack<Ts...>;

}}} /* end namespace vt::runtime::component */

#include "vt/runtime/component/component_pack.impl.h"

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_H*/
