/*
//@HEADER
// *****************************************************************************
//
//                                epoch_manip.h
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

#if !defined INCLUDED_VT_EPOCH_EPOCH_MANIP_H
#define INCLUDED_VT_EPOCH_EPOCH_MANIP_H

#include "vt/config.h"
#include "vt/epoch/epoch.h"
#include "vt/epoch/epoch_window.h"
#include "vt/termination/epoch_tags.h"
#include "vt/termination/interval/integral_set.h"
#include "vt/utils/bits/bits_common.h"
#include "vt/utils/bits/bits_packer.h"
#include "vt/runtime/component/component.h"

#include <unordered_map>

namespace vt { namespace epoch {

/** \file */

/**
 * \struct EpochManip epoch_manip.h vt/epoch/epoch_manip.h
 *
 * \brief Component for managing epoch ID allocation/deallocation and
 * manipulating the bits inside an epoch identifier.
 *
 * Used by the system mostly to manage the bits inside an \c EpochType. It knows
 * how to set the appropriate bits to change the type bits of an \c EpochType
 * by setting the bit pattern.
 */
struct EpochManip : runtime::component::Component<EpochManip> {
  using CapturedContextType = term::ParentEpochCapture;

  EpochManip();

  std::string name() override { return "EpochManip"; }

  /*
   *  Epoch getters to check type and state of EpochType
   */

  /**
   * \brief Gets whether the epoch is rooted or not
   *
   * \param[in] epoch the epoch to operate on
   *
   * \return whether the \c epoch is rooted
   */
  static bool isRooted(EpochType const& epoch);

  /**
   * \brief Gets the \c eEpochCategory of a given epoch
   *
   * \param[in] epoch the epoch to operate on
   *
   * \return the category of the \c epoch
   */
  static eEpochCategory category(EpochType const& epoch);

  /**
   * \brief Gets the node for the epoch (only relevant for rooted)
   *
   * \param[in] epoch the epoch to operate on
   *
   * \return the node (arbitrator) for the \c epoch
   */
  static NodeType node(EpochType const& epoch);

  /**
   * \brief Gets the sequential ID for an epoch
   *
   * \param[in] epoch the epoch to operate on
   *
   * \return the sequential number for an \c epoch
   */
  static EpochType::ImplType seq(EpochType const& epoch);

  /*
   *  Epoch setters to manipulate the type and state of EpochType
   */

  /**
   * \brief Set whether the \c epoch is rooted or not
   *
   * \param[in,out] epoch the epoch to modify
   * \param[in] is_rooted whether to set the epoch as rooted or not
   */
  static void setIsRooted(EpochType& epoch, bool const is_rooted);

  /**
   * \brief Set the category for the \c epoch
   *
   * \param[in,out] epoch the epoch to modify
   * \param[in] cat whether to set the epoch as rooted or not
   */
  static void setCategory(EpochType& epoch, eEpochCategory const cat);

  /**
   * \brief Set the node for a rooted \c epoch
   *
   * \param[in,out] epoch the epoch to modify
   * \param[in] node whether to set the epoch as rooted or not
   */
  static void setNode(EpochType& epoch, NodeType const node);

  /**
   * \brief Set the sequential ID for an \c epoch
   *
   * \param[in,out] epoch the epoch to modify
   * \param[in] seq the sequential ID to set on the epoch
   */
  static void setSeq(EpochType& epoch, EpochType::ImplType const seq);

  /*
   * General (stateless) methods for creating a epoch with certain properties
   * based on a current sequence number
   */

  /**
   * \brief Generate the control bits for a rooted epoch
   *
   * \param[in] category the category for the epoch
   *
   * \return the newly created epoch
   */
  static EpochType generateRootedEpoch(
    eEpochCategory const& category   = default_epoch_category
  );

  /**
   * \brief Generate the control bits for an epoch type (rooted or collective)
   *
   * \param[in] is_rooted if the epoch should be rooted or not
   * \param[in] root_node the root node for the epoch if \c is_rooted
   * \param[in] category the category for the epoch
   *
   * \return the newly created epoch
   */
  static EpochType generateEpoch(
    bool           const& is_rooted  = false,
    NodeType       const& root_node  = default_epoch_node,
    eEpochCategory const& category   = default_epoch_category
  );

  /*
   * Stateful methods for generating a new epoch with certain properties
   * using the next sequence number available
   */

  /**
   * \brief Stateful method to create the next rooted epoch
   *
   * \param[in] category the category for the epoch
   *
   * \return the newly created epoch
   */
  EpochType getNextRootedEpoch(
    eEpochCategory const& category = default_epoch_category
  );

  /**
   * \brief Stateful method to create the next rooted epoch with a particular
   * node embedded in the bits
   *
   * \param[in] category the category for the epoch
   * \param[in] root_node the root node for the epoch
   *
   * \return the newly created epoch
   */
  EpochType getNextRootedEpoch(
    eEpochCategory const& category, NodeType const root_node
  );

  /**
   * \brief Stateful method to create the next collective epoch
   *
   * \param[in] category the category for the epoch
   *
   * \return the newly created epoch
   */
  EpochType getNextCollectiveEpoch(
    eEpochCategory const& category = default_epoch_category
  );

public:
  /**
   * \brief Get an appropriate window that stores the list of
   * terminated epoch based on the epoch archetype
   *
   * \param[in] epoch the epoch in question
   *
   * \return the window of terminated epochs of that kind
   */
  EpochWindow* getTerminatedWindow(EpochType epoch);

  /**
   * \internal \brief Get archetype bits embedded in epoch
   *
   * \param[in] epoch the epoch in question
   *
   * \return all bits except for archetype bits masked out
   */
  EpochType getArchetype(EpochType epoch) const;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | terminated_epochs_
      | terminated_collective_epochs_;
  }

private:
  // epoch window container for specific archetyped epochs
  std::unordered_map<EpochType,std::unique_ptr<EpochWindow>> terminated_epochs_;
  // epoch window for basic collective epochs
  std::unique_ptr<EpochWindow> terminated_collective_epochs_ = nullptr;
};

}} /* end namespace vt::epoch */

namespace vt {

extern epoch::EpochManip* theEpoch();

}  //end namespace vt

#endif /*INCLUDED_VT_EPOCH_EPOCH_MANIP_H*/
