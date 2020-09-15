/*
//@HEADER
// *****************************************************************************
//
//                                epoch_manip.h
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

#if !defined INCLUDED_EPOCH_EPOCH_MANIP_H
#define INCLUDED_EPOCH_EPOCH_MANIP_H

#include "vt/config.h"
#include "vt/epoch/epoch.h"
#include "vt/utils/bits/bits_common.h"
#include "vt/utils/bits/bits_packer.h"

namespace vt { namespace epoch {

/** \file */

/// The default epoch node used for non-rooted epochs
static constexpr NodeType const default_epoch_node = uninitialized_destination;
/// The default epoch category
static constexpr eEpochCategory const default_epoch_category =
  eEpochCategory::NoCategoryEpoch;

/**
 * \struct EpochManip epoch_manip.h vt/epoch/epoch_manip.h
 *
 * \brief Class used to manipulate the bits in a \c EpochType and manage the
 * current sequential IDs for allocating epochs
 *
 * Used by the system mostly to manage the bits inside an \c EpochType. It knows
 * how to set the appropriate bits to change the static type of an \c EpochType
 * by setting the bit pattern.
 */
struct EpochManip {
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
   * \brief Gets whether the epoch has a category or not
   *
   * \param[in] epoch the epoch to operate on
   *
   * \return whether \c epoch has a category---
   *         not \c eEpochCategory::NoCategoryEpoch
   */
  static bool hasCategory(EpochType const& epoch);

  /**
   * \brief Gets whether the epoch is a user epoch (specifically a
   * user-customized dispatched epoch)
   *
   * \param[in] epoch the epoch to operate on
   *
   * \return whether the \c epoch is a user epoch
   */
  static bool isUser(EpochType const& epoch);

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
  static EpochType seq(EpochType const& epoch);

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
   * \brief Set whether the \c epoch has a category or not
   *
   * \param[in,out] epoch the epoch to modify
   * \param[in] has_cat whether to the epoch has a category
   */
  static void setHasCategory(EpochType& epoch, bool const has_cat  );

  /**
   * \brief Set whether the \c epoch is a user epoch or not
   *
   * \param[in,out] epoch the epoch to modify
   * \param[in] is_user whether to set the epoch as user or not
   */
  static void setIsUser(EpochType& epoch, bool const is_user);

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
  static void setSeq(EpochType& epoch, EpochType const seq);

  /*
   * General (stateless) methods for creating a epoch with certain properties
   * based on a current sequence number
   */

  /**
   * \brief Make a rooted epoch with a given sequential ID
   *
   * \param[in] seq the sequential ID for the epoch
   * \param[in] is_user whether the epoch is a user epoch or not
   * \param[in] category the category for the epoch
   *
   * \return the newly created epoch
   */
  static EpochType makeRootedEpoch(
    EpochType      const& seq,
    bool           const& is_user    = false,
    eEpochCategory const& category   = default_epoch_category
  );

  /**
   * \brief Make a new epoch (rooted or collective) with a given sequential ID
   *
   * \param[in] seq the sequential ID for the epoch
   * \param[in] is_rooted if the epoch should be rooted or not
   * \param[in] root_node the root node for the epoch if \c is_rooted
   * \param[in] is_user whether the epoch is a user epoch or not
   * \param[in] category the category for the epoch
   *
   * \return the newly created epoch
   */
  static EpochType makeEpoch(
    EpochType      const& seq,
    bool           const& is_rooted  = false,
    NodeType       const& root_node  = default_epoch_node,
    bool           const& is_user    = false,
    eEpochCategory const& category   = default_epoch_category
  );

  /**
   * \brief Create the next rooted epoch, stateful
   *
   * \param[in] is_user whether the epoch is a user epoch or not
   * \param[in] category the category for the epoch
   *
   * \return the newly created epoch
   */
  static EpochType makeNewRootedEpoch(
    bool           const& is_user    = false,
    eEpochCategory const& category   = default_epoch_category
  );

  /**
   * \brief Create the next epoch (rooted or collective), stateful
   *
   * \param[in] is_rooted if the epoch should be rooted or not
   * \param[in] root_node the root node for the epoch if \c is_rooted
   * \param[in] is_user whether the epoch is a user epoch or not
   * \param[in] category the category for the epoch
   *
   * \return the newly created epoch
   */
  static EpochType makeNewEpoch(
    bool           const& is_rooted  = false,
    NodeType       const& root_node  = default_epoch_node,
    bool           const& is_user    = false,
    eEpochCategory const& category   = default_epoch_category
  );

  /*
   * Stateful methods for creating a epoch based on epochs that have already
   * been created in the past
   */

  /**
   * \brief The next epoch given an epoch, increments the sequential ID
   *
   * \param[in] epoch the epoch to start from
   *
   * \return the newly created epoch
   */
  static EpochType next(EpochType const& epoch);

private:
  static EpochType nextSlow(EpochType const& epoch);
  static EpochType nextFast(EpochType const& epoch);

private:
  static EpochType cur_rooted_;     /**< The current rooted sequential ID  */
  static EpochType cur_non_rooted_; /**< The current non-rooted sequential ID */
};

}} /* end namespace vt::epoch */

#include "vt/epoch/epoch_manip_get.h"
#include "vt/epoch/epoch_manip_set.h"
#include "vt/epoch/epoch_manip_make.h"

#endif /*INCLUDED_EPOCH_EPOCH_MANIP_H*/
