/*
//@HEADER
// *****************************************************************************
//
//                               component_pack.h
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
   * pack.
   *
   * Component dependencies are specified with variadic template parameters in
   * \c StartupDepsPack and \c RuntimeDepsPack. Registration does not imply the
   * component will be created; it must be added subsequently to be enabled. It
   * simply declares its existence and connectivity with other components.
   *
   * Startup dependencies are ones that are required for the component to be
   * constructed and fire the initialize method. Additionally, and importantly,
   * they determine the order (reverse topological) in which components are torn
   * down or finalized. Both orders are sensitive as components require other
   * components to be live during construction/initialization and during
   * finalization. Runtime dependencies indicate that a component requires
   * another component to function and thus needs them to be loaded. Thus, for
   * instance, if a VT runtime is created with just an \c ActiveMessenger, the
   * runtime will add all the startup dependencies and runtime dependencies to
   * create a runtime with a working \c ActiveMessenger that is able to
   * function.
   *
   * \param[out] ref dumb pointer for access outside
   * \param[in] cons constructor arguments for the component---bound at
   * registration time
   *
   * \return \c registry::AutoHandlerType with type ID for component
   */
  template <
    typename T,
    typename... StartupDeps,
    typename... RuntimeDeps,
    typename... Cons
  >
  registry::AutoHandlerType registerComponent(
    T** ref,
    typename BaseComponent::StartupDepsPack<StartupDeps...>,
    typename BaseComponent::RuntimeDepsPack<RuntimeDeps...>,
    Cons&&... cons
  );

  /**
   * \internal \brief Add a component to the pack. It will be constructed along
   * with all its dependencies. It must be registered via \c registerComponent
   * before adding.
   */
  template <typename T>
  void add();

  /**
   * \internal \brief Construct all added components along with dependencies
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
   * \param[in] current_time current time
   *
   * \return the number of work units processed
   */
  int progress(TimeType current_time);

  /**
   * \internal \brief Needs current time
   *
   * \return whether any component needs the current time on the progress call
   */
  bool needsCurrentTime();

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
   * valid activation order based on registered dependencies.
   *
   * \return list of handlers representing a valid total ordering that does not
   * break dependencies
   */
  std::list<int> topoSort();

  /**
   * \internal \brief Topologically sort all the registered components to find a
   * valid activation order based on registered dependencies.
   *
   * \param[in] v current vertex
   * \param[in] order topological order derived so far
   * \param[in] visited array of visited vertices
   * \param[in] visiting array of vertices currently being visited
   */
  void topoSortImpl(int v, std::list<int>& order, bool* visited, bool* visiting);

  /**
   * \brief Based on the components that have been added along with their
   * startup and runtime dependencies, add any additional components that are
   * required for a complete VT runtime
   */
  void addAllRequiredComponents();

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

/**
 * \brief Type alias for a pack of startup dependencies
 */
template <typename... Ts>
using StartupDeps = typename BaseComponent::StartupDepsPack<Ts...>;

/**
 * \brief Type alias for a pack of runtime dependencies, automatically including
 * all startup dependencies
 */
template <typename... Ts>
using RuntimeDeps = typename BaseComponent::RuntimeDepsPack<Ts...>;

}}} /* end namespace vt::runtime::component */

#include "vt/runtime/component/component_pack.impl.h"

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_PACK_H*/
