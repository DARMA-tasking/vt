/*
//@HEADER
// *****************************************************************************
//
//                                stats_data.cc
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

#include "vt/vrt/collection/balance/stats_data.h"
#include "vt/context/context.h"

#include <nlohmann/json.hpp>

namespace vt { namespace vrt { namespace collection { namespace balance {

std::unique_ptr<nlohmann::json> StatsData::toJson(PhaseType phase) const {
  using json = nlohmann::json;

  json j;
  j["id"] = phase;

  std::size_t i = 0;
  if (node_data_.find(phase) != node_data_.end()) {
    for (auto&& elm : node_data_.at(phase)) {
      ElementIDStruct id = elm.first;
      TimeType time = elm.second;
      j["tasks"][i]["resource"] = "cpu";
      j["tasks"][i]["node"] = theContext()->getNode();
      j["tasks"][i]["time"] = time;
      j["tasks"][i]["entity"]["type"] = "object";
      j["tasks"][i]["entity"]["id"] = id.id;
      j["tasks"][i]["entity"]["home"] = id.home_node;
      if (node_idx_.find(id) != node_idx_.end()) {
        auto const& proxy_id = std::get<0>(node_idx_.find(id)->second);
        auto const& idx_vec = std::get<1>(node_idx_.find(id)->second);
        j["tasks"][i]["entity"]["collection_id"] = proxy_id;
        for (std::size_t x = 0; x < idx_vec.size(); x++) {
          j["tasks"][i]["entity"]["index"][x] = idx_vec[x];
        }
      }

      if (node_subphase_data_.find(phase) != node_subphase_data_.end()) {
        if (node_subphase_data_.at(phase).find(id) != node_subphase_data_.at(phase).end()) {
          auto const& subphase_times = node_subphase_data_.at(phase).at(id);
          std::size_t const subphases = subphase_times.size();
          if (subphases != 0) {
            for (std::size_t s = 0; s < subphases; s++) {
              j["tasks"][i]["subphases"][s]["id"] = s;
              j["tasks"][i]["subphases"][s]["time"] = subphase_times[s];
            }
          }
        }
      }
      i++;
    }
  }

  i = 0;
  if (node_comm_.find(phase) != node_comm_.end()) {
    for (auto&& elm : node_comm_.at(phase)) {
      auto volume = elm.second;
      auto const& key = elm.first;
      j["communications"][i]["bytes"] = volume.bytes;
      j["communications"][i]["messages"] = volume.messages;

      switch(key.cat_) {
      case CommCategory::Broadcast:
      case CommCategory::SendRecv: {
        if (key.cat_ == CommCategory::SendRecv) {
          j["communications"][i]["type"] = "SendRecv";
        } else {
          j["communications"][i]["type"] = "Broadcast";
        }
        j["communications"][i]["from"]["type"] = "object";
        j["communications"][i]["from"]["id"] = key.fromObj().id;
        j["communications"][i]["from"]["home"] = key.fromObj().home_node;
        j["communications"][i]["to"]["type"] = "object";
        j["communications"][i]["to"]["id"] = key.toObj().id;
        j["communications"][i]["to"]["home"] = key.toObj().home_node;
        break;
      }
      case CommCategory::NodeToCollection:
      case CommCategory::NodeToCollectionBcast: {
        if (key.cat_ == CommCategory::NodeToCollection) {
          j["communications"][i]["type"] = "NodeToCollection";
        } else {
          j["communications"][i]["type"] = "NodeToCollectionBcast";
        }

        j["communications"][i]["from"]["type"] = "node";
        j["communications"][i]["from"]["id"] = key.fromNode();
        j["communications"][i]["to"]["type"] = "object";
        j["communications"][i]["to"]["id"] = key.toObj().id;
        j["communications"][i]["to"]["home"] = key.toObj().home_node;
        break;
      }
      case CommCategory::CollectionToNode:
      case CommCategory::CollectionToNodeBcast: {
        if (key.cat_ == CommCategory::CollectionToNode) {
          j["communications"][i]["type"] = "CollectionToNode";
        } else {
          j["communications"][i]["type"] = "CollectionToNodeBcast";
        }

        j["communications"][i]["to"]["type"] = "node";
        j["communications"][i]["to"]["id"] = key.toNode();
        j["communications"][i]["from"]["type"] = "object";
        j["communications"][i]["from"]["id"] = key.fromObj().id;
        j["communications"][i]["from"]["home"] = key.fromObj().home_node;
        break;
      }
      case CommCategory::LocalInvoke:
      case CommCategory::CollectiveToCollectionBcast:
        // not currently supported
        break;
      }
      i++;
    }
  }

  return std::make_unique<json>(std::move(j));
}

StatsData::StatsData(nlohmann::json const& j) {
  auto this_node = theContext()->getNode();

  auto phases = j["phases"];
  if (phases.is_array()) {
    for (auto const& phase : phases) {
      auto id = phase["id"];
      auto tasks = phase["tasks"];

      this->node_data_[id];
      this->node_comm_[id];

      if (tasks.is_array()) {
        for (auto const& task : tasks) {
          auto node = task["node"];
          auto time = task["time"];
          auto etype = task["entity"]["type"];
          vtAssertExpr(time.is_number());
          vtAssertExpr(node.is_number());
          vtAssertExpr(node == this_node);

          if (etype == "object") {
            auto object = task["entity"]["id"];
            vtAssertExpr(object.is_number());

            NodeType home = uninitialized_destination;
            if (task["entity"].find("home") != task["entity"].end()) {
              auto home_json = task["entity"]["home"];
              vtAssertExpr(home_json.is_number());
              home = home_json;
            }

            auto elm = ElementIDStruct{object, home, node};
            this->node_data_[id][elm] = time;

            if (
              task["entity"].find("collection_id") != task["entity"].end() and
              task["entity"].find("index") != task["entity"].end()
            ) {
              auto cid = task["entity"]["collection_id"];
              auto idx = task["entity"]["index"];
              if (cid.is_number() && idx.is_array()) {
                std::vector<uint64_t> arr = idx;
                auto proxy = static_cast<VirtualProxyType>(cid);
                this->node_idx_[elm] = std::make_tuple(proxy, arr);
              }
            }

            if (task.find("subphases") != task.end()) {
              auto subphases = task["subphases"];
              if (subphases.is_array()) {
                for (auto const& s : subphases) {
                  auto sid = s["id"];
                  auto stime = s["time"];

                  vtAssertExpr(sid.is_number());
                  vtAssertExpr(stime.is_number());

                  this->node_subphase_data_[id][elm].resize(
                    static_cast<std::size_t>(sid) + 1
                  );
                  this->node_subphase_data_[id][elm][sid] = stime;
                }
              }
            }
          }
        }
      }

      if (phase.find("communications") != phase.end()) {
        auto comms = phase["communications"];
        if (comms.is_array()) {
          for (auto const& comm : comms) {
            auto bytes = comm["bytes"];
            auto messages = comm["messages"];
            auto type = comm["type"];

            vtAssertExpr(bytes.is_number());
            vtAssertExpr(messages.is_number());

            if (type == "SendRecv" || type == "Broadcast") {
              vtAssertExpr(comm["from"]["type"] == "object");
              vtAssertExpr(comm["to"]["type"] == "object");

              auto from_object = comm["from"]["id"];
              vtAssertExpr(from_object.is_number());
              NodeType from_home = uninitialized_destination;
              if (comm["from"].find("home") != comm["from"].end()) {
                auto home_json = comm["from"]["home"];
                vtAssertExpr(home_json.is_number());
                from_home = home_json;
              }
              auto from_elm = ElementIDStruct{from_object, from_home, this_node};

              auto to_object = comm["to"]["id"];
              vtAssertExpr(to_object.is_number());
              NodeType to_home = uninitialized_destination;
              if (comm["to"].find("home") != comm["to"].end()) {
                auto home_json = comm["to"]["home"];
                vtAssertExpr(home_json.is_number());
                to_home = home_json;
              }
              auto to_elm = ElementIDStruct{to_object, to_home, this_node};

              LBCommKey key(
                LBCommKey::CollectionTag{},
                from_elm, to_elm, type == "Broadcast"
              );
              CommVolume vol{bytes, messages};
              this->node_comm_[id][key] = vol;
            } else if (
              type == "NodeToCollection" || type == "NodeToCollectionBcast"
            ) {
              vtAssertExpr(comm["from"]["type"] == "node");
              vtAssertExpr(comm["to"]["type"] == "object");

              auto from_node = comm["from"]["id"];
              vtAssertExpr(from_node.is_number());

              auto to_object = comm["to"]["id"];
              vtAssertExpr(to_object.is_number());
              NodeType to_home = uninitialized_destination;
              if (comm["to"].find("home") != comm["to"].end()) {
                auto home_json = comm["to"]["home"];
                vtAssertExpr(home_json.is_number());
                to_home = home_json;
              }
              auto to_elm = ElementIDStruct{to_object, to_home, this_node};

              LBCommKey key(
                LBCommKey::NodeToCollectionTag{},
                static_cast<NodeType>(from_node), to_elm,
                type == "NodeToCollectionBcast"
              );
              CommVolume vol{bytes, messages};
              this->node_comm_[id][key] = vol;
            } else if (
              type == "CollectionToNode" || type == "CollectionToNodeBcast"
            ) {
              vtAssertExpr(comm["from"]["type"] == "object");
              vtAssertExpr(comm["to"]["type"] == "node");

              auto from_object = comm["from"]["id"];
              vtAssertExpr(from_object.is_number());
              NodeType from_home = uninitialized_destination;
              if (comm["from"].find("home") != comm["from"].end()) {
                auto home_json = comm["from"]["home"];
                vtAssertExpr(home_json.is_number());
                from_home = home_json;
              }
              auto from_elm = ElementIDStruct{from_object, from_home, this_node};

              auto to_node = comm["to"]["id"];
              vtAssertExpr(to_node.is_number());

              LBCommKey key(
                LBCommKey::CollectionToNodeTag{},
                from_elm, static_cast<NodeType>(to_node),
                type == "CollectionToNodeBcast"
              );
              CommVolume vol{bytes, messages};
              this->node_comm_[id][key] = vol;
            }
          }
        }
      }
    }
  }

  // @todo: implement subphase communication de-serialization, no use for it
  // right now, so it will be ignored
}

void StatsData::clear() {
  node_comm_.clear();
  node_data_.clear();
  node_subphase_data_.clear();
  node_subphase_comm_.clear();
  node_idx_.clear();
}

}}}} /* end namespace vt::vrt::collection::balance */
