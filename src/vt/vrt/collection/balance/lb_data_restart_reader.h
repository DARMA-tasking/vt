/*
//@HEADER
// *****************************************************************************
//
//                           lb_data_restart_reader.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_DATA_RESTART_READER_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_DATA_RESTART_READER_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/baselb/baselb_msgs.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/vrt/collection/balance/lb_data_holder.h"

#include <deque>
#include <map>
#include <string>
#include <vector>

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \struct LBDataRestartReader
 *
 * \brief A VT component for reading a user-input mapping file for object to
 * node.
 *
 * A common flow is for the LBAF (Load Balancing Analysis Framework) to generate
 * a new load distribution offline, producing LB data file, which are then read
 * by this component to follow the LB distribution described in those mapping
 * files.
 */
struct LBDataRestartReader : runtime::component::Component<LBDataRestartReader> {
  using ReduceMsg = collective::ReduceTMsg<std::vector<bool>>;

public:
  LBDataRestartReader() = default;

  void setProxy(objgroup::proxy::Proxy<LBDataRestartReader> in_proxy);

  static std::unique_ptr<LBDataRestartReader> construct();

  std::string name() override { return "LBDataRestartReader"; }

  void startup() override;

  /**
   * \brief Read LB data from a string stream
   *
   * \param[in] stream the input stream
   */
  void readLBDataFromStream(std::stringstream stream);

  /**
   * \brief Read LB data from the file specified
   *
   * \param[in] file the file
   */
  void readLBData(std::string const& fileName);

  /**
   * \brief Read the element history from an LB holder
   *
   * \param[in] lbdh the LB data holder
   */
  void readHistory(LBDataHolder const& lbdh);

  /**
   * \brief Return whether a phase needs LB
   *
   * \param[in] phase the phase
   *
   * \return whether it needs LB
   */
  bool needsLB(PhaseType phase) const {
    return changed_distro_.size() > phase ? changed_distro_.at(phase) : false;
  }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | proxy_
      | changed_distro_
      | history_
      | num_phases_;
  }

  /**
   * \brief Get the elements assigned for a given phase
   *
   * \param[in] phase the phase
   *
   * \return element assigned to this node
   */
  std::set<ElementIDStruct> const& getDistro(PhaseType phase) {
    auto iter = history_.find(phase);
    vtAssert(iter != history_.end(), "Must have a valid phase");
    return iter->second;
  }

  /**
   * \brief Clear history for a given phase
   *
   * \param[in] phase the phase to clear
   */
  void clearDistro(PhaseType phase) {
    auto iter = history_.find(phase);
    if (iter != history_.end()) {
      history_.erase(iter);
    }
  }

private:
  /**
   * \brief Reduce distribution changes globally to find where migrations need
   * to occur
   *
   * \param[in] vec the vector of booleans
   */
  void reduceDistroChanges(std::vector<bool> const& vec);

  /**
   * \brief Determine which phases migrations must happen to follow the
   * distribution
   */
  void determinePhasesToMigrate();

private:
  objgroup::proxy::Proxy<LBDataRestartReader> proxy_; /**< Objgroup proxy */

  /// Whether the distribution changed for a given phase and thus needs LB
  std::vector<bool> changed_distro_;

  /// History of mapping that was read in from the data files
  std::unordered_map<PhaseType, std::set<ElementIDStruct>> history_;

  struct DepartMsg : vt::Message {
    DepartMsg(NodeType in_depart_node, PhaseType in_phase, ElementIDStruct in_elm)
      : depart_node(in_depart_node),
        phase(in_phase),
        elm(in_elm)
    { }

    NodeType depart_node = uninitialized_destination;
    PhaseType phase = no_lb_phase;
    ElementIDStruct elm;
  };

  struct ArriveMsg : vt::Message {
    ArriveMsg(NodeType in_arrive_node, PhaseType in_phase, ElementIDStruct in_elm)
      : arrive_node(in_arrive_node),
        phase(in_phase),
        elm(in_elm)
    { }

    NodeType arrive_node = uninitialized_destination;
    PhaseType phase = no_lb_phase;
    ElementIDStruct elm;
  };

  struct UpdateMsg : vt::Message {
    UpdateMsg(NodeType in_curr_node, PhaseType in_phase, ElementIDStruct in_elm)
      : curr_node(in_curr_node),
        phase(in_phase),
        elm(in_elm)
    { }

    NodeType curr_node = uninitialized_destination;
    PhaseType phase = no_lb_phase;
    ElementIDStruct elm;
  };

  struct Coord {
    MsgSharedPtr<ArriveMsg> arrive = nullptr;
    MsgSharedPtr<DepartMsg> depart = nullptr;
  };

  void departing(DepartMsg* msg);
  void arriving(ArriveMsg* msg);
  void update(UpdateMsg* msg);
  void checkBothEnds(Coord& coord);

  std::unordered_map<
    PhaseType, std::unordered_map<ElementIDStruct, Coord>
  > coordinate_;

  /// Number of phases read in
  std::size_t num_phases_ = 0;
};

}}}} /* end namespace vt::vrt::collection::balance */

namespace vt {

extern vrt::collection::balance::LBDataRestartReader* theLBDataReader();

} /* end namespace vt */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_DATA_RESTART_READER_H*/
