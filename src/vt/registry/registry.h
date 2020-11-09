/*
//@HEADER
// *****************************************************************************
//
//                                  registry.h
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

#if !defined INCLUDED_REGISTRY_REGISTRY_H
#define INCLUDED_REGISTRY_REGISTRY_H

#include <vector>
#include <unordered_map>
#include <cassert>

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/handler/handler.h"
#include "vt/runtime/component/component.h"

namespace vt { namespace registry {

/**
 * \struct Registry
 *
 * \brief A VT component that manages registered active handlers.
 */
struct Registry : runtime::component::Component<Registry> {
  using HandlerManagerType = HandlerManager;
  using HandlerBitsType = eHandlerBits;
  using TaggerHandlerType = std::tuple<TagType, HandlerType>;
  using ContainerType = std::unordered_map<HandlerType, ActiveClosureFnType>;
  using TagContainerType = std::unordered_map<TagType, ActiveClosureFnType>;
  using HanTagContainerType = std::unordered_map<HandlerType, TagContainerType>;

  /**
   * \internal \brief System call to construct a registry
   */
  Registry() = default;

  std::string name() override { return "Registry"; }

  /**
   * \brief Register a new handler
   *
   * \param[in] fn the active function pointer
   * \param[in] tag relevant message tag for delivery
   * \param[in] is_collective whether it's a collective registration
   *
   * \return the handler ID
   */
  HandlerType registerNewHandler(
    ActiveClosureFnType fn, TagType const& tag = no_tag,
    bool const& is_collective = false
  );

  /**
   * \brief Unregister a handler
   *
   * \param[in] han the handler ID
   * \param[in] tag relevant message tag for delivery
   */
  void unregisterHandlerFn(
    HandlerType const han, TagType const& tag = no_tag
  );

  /**
   * \brief Swap underlying handler out for registered handler ID
   *
   * \param[in] han the handler ID
   * \param[in] fn the active function to attach
   * \param[in] tag tag to associate
   */
  void swapHandler(
    HandlerType const han, ActiveClosureFnType fn, TagType const& tag = no_tag
  );

  /**
   * \brief Registry an active handler
   *
   * \param[in] fn the active handler
   * \param[in] tag relevant message tag
   *
   * \return the handler ID
   */
  HandlerType registerActiveHandler(
    ActiveClosureFnType fn, TagType const& tag = no_tag
  );

  /**
   * \brief Get active function for handler
   *
   * \param[in] han the handler ID
   * \param[in] tag the relevant tag
   *
   * \return the active function
   */
  ActiveClosureFnType getHandler(
    HandlerType const han, TagType const& tag = no_tag
  );

  /**
   * \brief Get active function for handler
   *
   * \param[in] han the handler ID
   *
   * \return the active function
   */
  ActiveClosureFnType getHandlerNoTag(HandlerType const han);

private:
  ContainerType registered_;
  HanTagContainerType tagged_registered_;
  HandlerIdentifierType cur_ident_collective_ = first_handle_identifier;
  HandlerIdentifierType cur_ident_ = first_handle_identifier;
};

}} //end namespace vt::registry

namespace vt {

extern registry::Registry* theRegistry();

} // end namespace vt

#endif /*INCLUDED_REGISTRY_REGISTRY_H*/
