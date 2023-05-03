/*
//@HEADER
// *****************************************************************************
//
//                          lb_data_restart_reader.cc
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_DATA_RESTART_READER_CC
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_DATA_RESTART_READER_CC

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_data_restart_reader.h"
#include "vt/objgroup/manager.h"
#include "vt/vrt/collection/balance/lb_data_holder.h"
#include "vt/utils/json/json_reader.h"
#include "vt/utils/json/decompression_input_container.h"
#include "vt/utils/json/input_iterator.h"

#include <cinttypes>

#include <nlohmann/json.hpp>

namespace vt { namespace vrt { namespace collection { namespace balance {

void LBDataRestartReader::setProxy(
  objgroup::proxy::Proxy<LBDataRestartReader> in_proxy
) {
  proxy_ = in_proxy;
}

/*static*/ std::unique_ptr<LBDataRestartReader> LBDataRestartReader::construct() {
  auto ptr = std::make_unique<LBDataRestartReader>();
  auto proxy = theObjGroup()->makeCollective<LBDataRestartReader>(
    ptr.get(), "LBDataRestartReader"
  );
  proxy.get()->setProxy(proxy);
  return ptr;
}

void LBDataRestartReader::startup() {
  auto const file_name = theConfig()->getLBDataFileIn();
  readLBData(file_name);
}

void LBDataRestartReader::readHistory(LBDataHolder const& lbdh) {
  num_phases_ = lbdh.node_data_.size();
  for (PhaseType phase = 0; phase < num_phases_; phase++) {
    auto iter = lbdh.node_data_.find(phase);
    if (iter != lbdh.node_data_.end()) {
      for (auto const& obj : iter->second) {
        if (obj.first.isMigratable()) {
          history_[phase].insert(obj.first);
        }
      }
    } else {
      // We assume that all phases are dense all fully specified even if they
      // don't change
      vtAbort("Could not find data: phases must all be specified");
    }
  }
}

void LBDataRestartReader::readLBDataFromStream(std::stringstream stream) {
  using vt::util::json::DecompressionInputContainer;
  using vt::vrt::collection::balance::LBDataHolder;
  using json = nlohmann::json;

  auto c = DecompressionInputContainer(
    DecompressionInputContainer::AnyStreamTag{}, std::move(stream)
  );
  json j = json::parse(c);
  auto lbdh = LBDataHolder(j);
  readHistory(lbdh);
  determinePhasesToMigrate();
}

void LBDataRestartReader::readLBData(std::string const& file) {
  using vt::util::json::Reader;
  using vt::vrt::collection::balance::LBDataHolder;

  Reader r{file};
  auto json = r.readFile();
  auto lbdh = LBDataHolder(*json);
  readHistory(lbdh);
  determinePhasesToMigrate();
}

void LBDataRestartReader::departing(DepartMsg* msg) {
  auto m = promoteMsg(msg);
  coordinate_[msg->phase][msg->elm].depart = m;
  checkBothEnds(coordinate_[msg->phase][msg->elm]);
}

void LBDataRestartReader::arriving(ArriveMsg* msg) {
  auto m = promoteMsg(msg);
  coordinate_[msg->phase][msg->elm].arrive = m;
  checkBothEnds(coordinate_[msg->phase][msg->elm]);
}

void LBDataRestartReader::update(UpdateMsg* msg) {
  auto iter = history_[msg->phase].find(msg->elm);
  vtAssert(iter != history_[msg->phase].end(), "Must exist");
  auto elm = *iter;
  elm.curr_node = msg->curr_node;
  history_[msg->phase].erase(iter);
  history_[msg->phase].insert(elm);
}

void LBDataRestartReader::checkBothEnds(Coord& coord) {
  if (coord.arrive != nullptr and coord.depart != nullptr) {
    proxy_[coord.arrive->arrive_node].send<
      UpdateMsg, &LBDataRestartReader::update
    >(coord.depart->depart_node, coord.arrive->phase, coord.arrive->elm);
  }
}

void LBDataRestartReader::determinePhasesToMigrate() {
  std::vector<bool> local_changed_distro;
  local_changed_distro.resize(num_phases_ - 1);

  auto const this_node = theContext()->getNode();

  runInEpochCollective("LBDataRestartReader::updateLocations", [&]{
    for (PhaseType i = 0; i < num_phases_ - 1; ++i) {
      local_changed_distro[i] = history_[i] != history_[i+1];
      if (local_changed_distro[i]) {
        std::set<ElementIDStruct> departing, arriving;

        std::set_difference(
          history_[i+1].begin(), history_[i+1].end(),
          history_[i].begin(),   history_[i].end(),
          std::inserter(arriving, arriving.begin())
        );

        std::set_difference(
          history_[i].begin(),   history_[i].end(),
          history_[i+1].begin(), history_[i+1].end(),
          std::inserter(departing, departing.begin())
        );

        for (auto&& d : departing) {
          proxy_[d.getHomeNode()].send<DepartMsg, &LBDataRestartReader::departing>(this_node, i+1, d);
        }
        for (auto&& a : arriving) {
          proxy_[a.getHomeNode()].send<ArriveMsg, &LBDataRestartReader::arriving>(this_node, i+1, a);
        }
      }
    }
  });

  runInEpochCollective("LBDataRestartReader::computeDistributionChanges", [&]{
    proxy_.allreduce<
      &LBDataRestartReader::reduceDistroChanges, collective::OrOp
    >(std::move(local_changed_distro));
  });
}

void LBDataRestartReader::reduceDistroChanges(std::vector<bool> const& vec) {
  changed_distro_ = std::move(vec);
}

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_DATA_RESTART_READER_CC*/
