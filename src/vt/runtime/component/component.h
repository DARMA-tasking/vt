/*
//@HEADER
// *****************************************************************************
//
//                                 component.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_H
#define INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_H

#include "vt/configs/error/hard_error.h"
#include "vt/runtime/component/component_registry.h"
#include "vt/runtime/component/component_dep.h"
#include "vt/runtime/component/component_traits.h"
#include "vt/runtime/component/base.h"

#include <memory>

namespace vt { namespace runtime { namespace component {

/**
 * \struct Component component.h vt/runtime/component/component.h
 *
 * \brief \c Component class for a generic VT runtime module, CRTP'ed over the
 * component's actual type
 */
template <typename T>
struct Component : BaseComponent {

  Component() = default;

  /**
   * \brief Constructor for a new component with the appropriate typed
   * dependencies
   *
   * \param[in] DepsPack a pack of typed dependencies for the given component
   * that it depends on
   *
   * \return the new component
   */
  template <typename... Deps>
  Component(DepsPack<Deps...>) {
    ComponentRegistry::dependsOn<T, Deps...>();
  }

  /// Trait for components that have a specialized static construct method (used
  /// for objgroups)
  template <typename U>
  using hasCons = typename std::enable_if<ComponentTraits<U>::hasConstruct, T>::type;

  /// Trait for components that do not have a specialized static construct method
  template <typename U>
  using hasNoCons = typename std::enable_if<not ComponentTraits<U>::hasConstruct, T>::type;

  /**
   * \brief Construct the component with the specialized construct method
   *
   * \param[in] args the arguments to forward to the component's constructor
   *
   * \return unique pointer to the the component
   */
  template <typename... Args, typename U = T>
  static std::unique_ptr<T> staticInit(Args&&... args, hasCons<U>* = nullptr) {
    return T::construct(std::forward<Args>(args)...);
  }

  /**
   * \brief Construct the component with normal component constructor
   *
   * \param[in] args the arguments to forward to the component's constructor
   *
   * \return unique pointer to the the component
   */
  template <typename... Args, typename U = T>
  static std::unique_ptr<T> staticInit(Args&&... args, hasNoCons<U>* = nullptr) {
    return std::make_unique<T>(std::forward<Args>(args)...);
  }

  /**
   * \brief Whether the component requires the scheduler to poll
   *
   * \return default component is not pollable
   */
  bool pollable() override {
    return false;
  }

  /**
   * \brief Empty default overridden initialize method
   */
  virtual void initialize() override { }

  /**
   * \brief Empty default overridden finalize method
   */
  virtual void finalize() override { }

  /**
   * \brief Empty default overridden startup method
   */
  virtual void startup() override { }

  /**
   * \brief Empty default overridden progress method
   *
   * \return no units processed
   */
  virtual int progress() override { return 0; }

  /**
   * \brief Empty default diagnostic dump state
   */
  void dumpState() override { }
};

/**
 * \struct PollableComponent component.h vt/runtime/component/component.h
 *
 * \brief \c Component class for a generic, pollable VT runtime module, CRTP'ed
 * over the component's actual type. A pollable component will be registered
 * with the VT scheduler to ensure it makes progress.
 */
template <typename T>
struct PollableComponent : Component<T> {

  /**
   * \brief Whether the component requires the scheduler to poll
   *
   * \return pollable component returns true to indicate progress function to be
   * invoked
   */
  bool pollable() override {
    return true;
  }

  /**
   * \brief Override progress function to force user to supply a real
   * function. Abort if the user does not.
   *
   * \return number of units processed---zero
   */
  virtual int progress() override {
    vtAbort("PollableComponent should override the empty progress function");
    return 0;
  }

};

}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_H*/
