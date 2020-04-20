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

template <typename T>
struct Component : BaseComponent {

  Component() = default;

  template <typename... Deps>
  Component(DepsPack<Deps...>) {
    ComponentRegistry::dependsOn<T, Deps...>();
  }

  /// Traits for objgroup components which have a specialized static construct
  template <typename U>
  using hasCons = typename std::enable_if<ComponentTraits<U>::hasConstruct, T>::type;
  template <typename U>
  using hasNoCons = typename std::enable_if<not ComponentTraits<U>::hasConstruct, T>::type;

  template <typename... Args, typename U = T>
  static std::unique_ptr<T> staticInit(Args&&... args, hasCons<U>* = nullptr) {
    return T::construct(std::forward<Args>(args)...);
  }

  template <typename... Args, typename U = T>
  static std::unique_ptr<T> staticInit(Args&&... args, hasNoCons<U>* = nullptr) {
    return std::make_unique<T>(std::forward<Args>(args)...);
  }

  bool pollable() override {
    return false;
  }

  virtual void initialize() override { }
  virtual void finalize() override { }
  virtual void startup() override { }

  // Default empty progress function
  virtual int progress() override { return 0; }

  void dumpState() override {
    /* here to compile, should be implemented by each component*/
  }
};

template <typename T>
struct PollableComponent : Component<T> {

  bool pollable() override {
    return true;
  }

  // Fail if progress method not overridden by user
  virtual int progress() override {
    vtAbort("PollableComponent should have a progress function");
    return 0;
  }

};

}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_H*/
