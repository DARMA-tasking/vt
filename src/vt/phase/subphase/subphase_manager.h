/*
//@HEADER
// *****************************************************************************
//
//                              subphase_manager.h
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

#if !defined INCLUDED_VT_PHASE_SUBPHASE_SUBPHASE_MANAGER_H
#define INCLUDED_VT_PHASE_SUBPHASE_SUBPHASE_MANAGER_H

#include "vt/configs/types/types_type.h"
#include "vt/configs/types/types_sentinels.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace vt { namespace phase { namespace subphase {

// fwd-declare messages for resolution protocol
struct SubphaseIDMsg;
struct RootedStringMsg;

/**
 * \struct SubphaseManager
 *
 * \brief Trait for the \c PhaseManager component that provides functionality
 * for generating subphase IDs that are unique across all nodes from arbitrary
 * string labels.
 */
struct SubphaseManager {
  using SubphaseAction = std::function<void(SubphaseType)>;
  using ActionListType = std::vector<SubphaseAction>;

  SubphaseManager() = default;

public:
  /**
   * \brief Register a new collective subphase label
   *
   * \param[in] label the string subphase label
   *
   * \return the subphase ID
   */
  SubphaseType registerCollectiveSubphase(std::string const& label);

  /**
   * \brief Register a new rooted subphase label
   *
   * \param[in] label the string subphase label
   * \param[in] action action executed after resolution with the subphase ID
   */
  void registerRootedSubphase(std::string const& label, SubphaseAction action);

  /**
   * \internal
   * \brief Serialize for footprinting
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | subphase_proxy_
      | rooted_ids_
      | pending_actions_
      | pending_
      | collective_ids_
      | collective_used_ids_
      | resolved_broker_ids_;
  }

private:
  /**
   * \internal
   * \brief Handler to resolve the ID for a rooted subphase executed
   * on the broker node determined by hashing the label.
   *
   * \param[in] msg the message with the subphase label
   */
  void resolveRootedString(RootedStringMsg* msg);

protected:
  /**
   * \internal
   * \brief Construct the object group for the subphase manager; invoked by the
   * phase manager during startup.
   */
  static void subphaseConstruct();

  /**
   * \internal
   * \brief Inquire if the \c SubphaseManager is awaiting resolution of strings
   *  and has pending actions that should trigger
   *
   * \return whether it is pending resolution
   */
  bool isPendingResolution() const;

private:
  /// Objgroup proxy for the subphase manager
  ObjGroupProxyType subphase_proxy_ = no_obj_group;
  /// The rooted subphase IDs that have been resolved w/the broker
  std::unordered_map<std::string, SubphaseType> rooted_ids_;
  /// Pending actions for rooted subphases waiting on resolution
  std::unordered_map<std::string, ActionListType> pending_actions_;
  /// Set of pending labels for rooted subphases
  std::unordered_set<std::string> pending_;

private:
  /// The collective subphase IDs that have been computed already
  std::unordered_map<std::string, SubphaseType> collective_ids_;
  /// Set of collective hash IDs that already have been used
  std::unordered_set<SubphaseType> collective_used_ids_;

private:
  /// Resolved labels that the broker holds to send to inquiring nodes
  std::unordered_map<std::string, SubphaseType> resolved_broker_ids_;
  /// The next broker rooted ID to mix into the subphase ID bits
  SubphaseType broker_rooted_id_ = 0;
};

}}} /* end namespace vt::phase::subphase */

#endif /*INCLUDED_VT_PHASE_SUBPHASE_SUBPHASE_MANAGER_H*/
