/*
//@HEADER
// *****************************************************************************
//
//                               lb_data_holder.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_DATA_HOLDER_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_DATA_HOLDER_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_common.h"

#if vt_check_enabled(tv)
#  include <vt-tv/api/info.h>
#endif

#include <unordered_map>
#include <memory>

#include <nlohmann/json_fwd.hpp>

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \struct LBDataHolder
 *
 * \brief Data structure that holds LB data for a set of phases. Can
 * output them as JSON.
 */
struct LBDataHolder {
  LBDataHolder() = default;

  /**
   * \brief Create \c LBDataHolder from input JSON
   *
   * \param[in] j the json that contains the LB data
   */
  LBDataHolder(nlohmann::json const& j);

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | node_data_;
    s | node_comm_;
    s | node_subphase_comm_;
    s | user_defined_json_;
    s | user_per_phase_json_;
    s | node_idx_;
    s | skipped_phases_;
    s | identical_phases_;
    s | user_defined_lb_info_;
    s | node_user_attributes_;
    s | rank_attributes_;
  }

  /**
   * \brief Output a phase's LB data to JSON
   *
   * \param[in] phase the phase
   *
   * \return the json data structure
   */
  std::unique_ptr<nlohmann::json> toJson(PhaseType phase) const;

#if vt_check_enabled(tv)
  /**
   * \brief Generate vt-tv data structure for visualization
   *
   * \param[in] phase the phase to generate
   *
   * \return a \c vt::tv::PhaseWork data structure
   */
  std::unique_ptr<tv::PhaseWork> toTV(PhaseType phase) const;

  /**
   * \brief Get all object info mapped here for a given phase
   *
   * \return map with object info
   */
  std::unordered_map<ElementIDType, tv::ObjectInfo> getObjInfo(
    PhaseType phase
  ) const;
#endif

  /**
   * \brief Output a LB phase's metadata to JSON
   *
   * \return the json data structure
   */
  std::unique_ptr<nlohmann::json> metadataToJson() const;

  /**
   * \brief Output a LB rank attributes metadata to JSON
   *
   * \return the json data structure
   */
  std::unique_ptr<nlohmann::json> rankAttributesToJson() const;

  /**
   * \brief Clear all LB data
   */
  void clear();

private:
  /**
   * \brief Output an entity to json
   *
   * \param[in] j the json
   * \param[in] elm_id the element to output
   */
  void outputEntity(nlohmann::json& j, ElementIDStruct const& elm_id) const;

  void addInitialTask(nlohmann::json& j, std::size_t n) const;

  /**
   * \brief Determine the object ID from the tasks or communication field of
   *        input JSON
   *
   * \param[in] field the json field containing an object ID
   * \param[in] object empty json object to be populated with the object's ID
   * \param[in] is_bitpacked empty bool to be populated with whether or not
   *                         the ID is bit-encoded
   * \param[in] is_collection empty bool to be populated with whether
   *                          or not the object belongs to a collection
   */
  static void getObjectFromJsonField_(
    nlohmann::json const& field, nlohmann::json& object,
    bool& is_bitpacked, bool& is_collection);

  /**
   * \brief Create an ElementIDStruct for the communication object
   *
   * \param[in] field the communication field for the desired object
   *                  e.g. communications["to"] or communications["from"]
   */
  ElementIDStruct getElmFromCommObject_(nlohmann::json const& field) const;

  /**
   * \brief Read the LB phase's metadata
   *
   * \param[in] j the json
   */
  void readMetadata(nlohmann::json const& j);

public:
  /// The current node
  NodeType this_node_ = vt::uninitialized_destination;
  /// Node attributes for the current rank
  ElmUserDataType rank_attributes_;
  /// Node timings for each local object
  std::unordered_map<PhaseType, LoadMapType> node_data_;
  /// Node communication graph for each local object
  std::unordered_map<PhaseType, CommMapType> node_comm_;
  /// Node communication graph for each subphase
  std::unordered_map<PhaseType, std::unordered_map<SubphaseType, CommMapType>> node_subphase_comm_;
  /// User-defined data from each phase for JSON output
  std::unordered_map<PhaseType, std::unordered_map<
    ElementIDStruct, std::shared_ptr<nlohmann::json>
  >> user_defined_json_;

  std::unordered_map<PhaseType, std::shared_ptr<nlohmann::json>> user_per_phase_json_;
  /// User-defined data from each phase for LB
  std::unordered_map<PhaseType, DataMapType> user_defined_lb_info_;
  /// User-defined attributes from each phase
  std::unordered_map<PhaseType, DataMapType> node_user_attributes_;
  /// Node indices for each ID along with the proxy ID
  std::unordered_map<ElementIDStruct, std::tuple<VirtualProxyType, std::vector<uint64_t>>> node_idx_;
  /// Map from id to objgroup proxy
  std::unordered_map<ElementIDStruct, ObjGroupProxyType> node_objgroup_;
  // Set of phases that are skipped
  std::set<PhaseType> skipped_phases_;
  // Set of phases which are identical to previous
  std::set<PhaseType> identical_phases_;
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_DATA_HOLDER_H*/
