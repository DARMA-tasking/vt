/*
//@HEADER
// *****************************************************************************
//
//                              lb_data_holder.cc
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

#include "vt/context/context.h"
#include "vt/elm/elm_id_bits.h"
#include "vt/vrt/collection/balance/lb_data_holder.h"

#if vt_check_enabled(tv)
#  include <vt-tv/api/info.h>
#endif

#include <nlohmann/json.hpp>

namespace vt { namespace vrt { namespace collection { namespace balance {

void LBDataHolder::getObjectFromJsonField_(
  nlohmann::json const& field, nlohmann::json& object, bool& is_bitpacked,
  bool& is_collection) {
  if (field.find("id") != field.end()) {
    object = field["id"];
    is_bitpacked = true;
  } else {
    object = field["seq_id"];
    is_bitpacked = false;
  }
  vtAssertExpr(object.is_number());
  if (field.find("collection_id") != field.end()) {
    is_collection = true;
  } else {
    is_collection = false;
  }
}

ElementIDStruct
LBDataHolder::getElmFromCommObject_(
  nlohmann::json const& field) const {
  // Get the object's id and determine if it is bit-encoded
  nlohmann::json object;
  bool is_bitpacked, is_collection;
  getObjectFromJsonField_(field, object, is_bitpacked, is_collection);

  // Create elm with encoded data
  ElementIDStruct elm;
  if (is_collection and not is_bitpacked) {
    int home = field["home"];
    bool is_migratable = field["migratable"];
    elm = elm::ElmIDBits::createCollectionImpl(
      is_migratable, static_cast<ElementIDType>(object), home, this_node_);
  } else {
    elm = ElementIDStruct{object, this_node_};
  }

  return elm;
}

void LBDataHolder::outputEntity(nlohmann::json& j, ElementIDStruct const& id) const {
  j["type"] = "object";
  j["id"] = id.id;
  j["home"] = id.getHomeNode();
  j["migratable"] = id.isMigratable();
  if (node_idx_.find(id) != node_idx_.end()) {
    auto const& proxy_id = std::get<0>(node_idx_.find(id)->second);
    auto const& idx_vec = std::get<1>(node_idx_.find(id)->second);
    j["collection_id"] = proxy_id;
    for (std::size_t x = 0; x < idx_vec.size(); x++) {
      j["index"][x] = idx_vec[x];
    }
  } else if (node_objgroup_.find(id) != node_objgroup_.end()) {
    auto const& proxy_id = node_objgroup_.find(id)->second;
    j["objgroup_id"] = proxy_id;
  } else {
    // bare handler
  }
}

std::unique_ptr<nlohmann::json> LBDataHolder::metadataToJson() const {
  // Find last element of a range for which it's values are incremented by 1
  auto find_last_of_range = [](
                              const std::set<PhaseType>& notEmptySet,
                              std::set<PhaseType>::iterator fromIt) {
    vtAssert(!notEmptySet.empty(), "Input Set must be not empty");

    do {
      auto next = std::next(fromIt);
      // end of a set or range
      if (next == notEmptySet.end() || *fromIt + 1 != *next) {
        break;
      }
      ++fromIt;
    } while (fromIt != notEmptySet.end());

    return fromIt;
  };

  nlohmann::json j;

  // Generate list and ranges of skipped phases
  std::set<PhaseType> skipped_list;
  std::vector<std::pair<PhaseType, PhaseType>> skipped_ranges;
  for (auto it = skipped_phases_.begin(); it != skipped_phases_.end(); it++) {
    auto endOfRange = find_last_of_range(skipped_phases_, it);
    if (it == endOfRange) {
      skipped_list.insert(*it);
    } else {
      skipped_ranges.emplace_back(*it, *endOfRange);
      it = endOfRange;
    }
  }

  // Generate list and ranges of identical phases
  std::set<PhaseType> identical_list;
  std::vector<std::pair<PhaseType, PhaseType>> identical_ranges;
  for (auto it = identical_phases_.begin(); it != identical_phases_.end();
       it++) {
    auto endOfRange = find_last_of_range(identical_phases_, it);
    if (it == endOfRange) {
      identical_list.insert(*it);
    } else {
      identical_ranges.emplace_back(*it, *endOfRange);
      it = endOfRange;
    }
  }

  // Save metadata
  j["skipped"]["list"] = skipped_list;
  j["skipped"]["range"] = skipped_ranges;
  j["identical_to_previous"]["list"] = identical_list;
  j["identical_to_previous"]["range"] = identical_ranges;
  return std::make_unique<nlohmann::json>(std::move(j));
}

std::unique_ptr<nlohmann::json> LBDataHolder::rankAttributesToJson() const {
  if (rank_attributes_.empty()) {
    return nullptr;
  }

  nlohmann::json j;

  for (auto const& [key, value] : rank_attributes_) {
    if (std::holds_alternative<int>(value)) {
      j["attributes"][key] = std::get<int>(value);
    } else if (std::holds_alternative<double>(value)) {
      j["attributes"][key] = std::get<double>(value);
    } else if (std::holds_alternative<std::string>(value)) {
      j["attributes"][key] = std::get<std::string>(value);
    }
  }

  return std::make_unique<nlohmann::json>(std::move(j));
}

void LBDataHolder::addInitialTask(nlohmann::json& j, std::size_t n) const {
  j["tasks"][n]["resource"] = "cpu";
  j["tasks"][n]["node"] = vt::theContext()->getNode();
  j["tasks"][n]["time"] = 0.0;
  outputEntity(
    j["tasks"][n]["entity"], ElementIDStruct()
  );
}

std::unique_ptr<nlohmann::json> LBDataHolder::toJson(PhaseType phase) const {
  using json = nlohmann::json;

  json j;
  j["id"] = phase;

  std::size_t i = 0;
  if (node_data_.find(phase) != node_data_.end()) {
    for (auto&& elm : node_data_.at(phase)) {
      ElementIDStruct id = elm.first;
      LoadType time = elm.second.whole_phase_load;
      j["tasks"][i]["resource"] = "cpu";
      j["tasks"][i]["node"] = id.getCurrNode();
      j["tasks"][i]["time"] = time;
      if (user_defined_json_.find(phase) != user_defined_json_.end()) {
        auto &user_def_this_phase = user_defined_json_.at(phase);
        if (user_def_this_phase.find(id) != user_def_this_phase.end()) {
          auto &user_def = user_def_this_phase.at(id);
          if (!user_def->empty()) {
            j["tasks"][i]["user_defined"] = *user_def;
          }
        }
      }
      outputEntity(j["tasks"][i]["entity"], id);

      if (node_user_attributes_.find(phase) != node_user_attributes_.end()) {
        if (node_user_attributes_.at(phase).find(id) != node_user_attributes_.at(phase).end()) {
          for (auto const& [key, value] : node_user_attributes_.at(phase).at(id)) {
            if (std::holds_alternative<int>(value)) {
              j["tasks"][i]["attributes"][key] = std::get<int>(value);
            } else if (std::holds_alternative<double>(value)) {
              j["tasks"][i]["attributes"][key] = std::get<double>(value);
            } else if (std::holds_alternative<std::string>(value)) {
              j["tasks"][i]["attributes"][key] = std::get<std::string>(value);
            }
          }
        }
      }

      auto const& subphase_times = elm.second.subphase_loads;
      std::size_t const subphases = subphase_times.size();
      if (subphases != 0) {
        for (std::size_t s = 0; s < subphases; s++) {
          j["tasks"][i]["subphases"][s]["id"] = s;
          j["tasks"][i]["subphases"][s]["time"] = subphase_times[s];
        }
      }

      i++;
    }

    if ((phase == 0) and (i > 0)) {
      addInitialTask(j, i);
    }
  }

  i = 0;
  if (node_comm_.find(phase) != node_comm_.end()) {
    for (auto const& [key, volume] : node_comm_.at(phase)) {
      j["communications"][i]["bytes"] = volume.bytes;
      j["communications"][i]["messages"] = volume.messages;

      switch(key.cat_) {
      case elm::CommCategory::Broadcast:
      case elm::CommCategory::SendRecv: {
        if (key.cat_ == elm::CommCategory::SendRecv) {
          j["communications"][i]["type"] = "SendRecv";
        } else {
          j["communications"][i]["type"] = "Broadcast";
        }
        outputEntity(j["communications"][i]["from"], key.fromObj());
        outputEntity(j["communications"][i]["to"], key.toObj());
        break;
      }
      case elm::CommCategory::NodeToCollection:
      case elm::CommCategory::NodeToCollectionBcast: {
        if (key.cat_ == elm::CommCategory::NodeToCollection) {
          j["communications"][i]["type"] = "NodeToCollection";
        } else {
          j["communications"][i]["type"] = "NodeToCollectionBcast";
        }

        j["communications"][i]["from"]["type"] = "node";
        j["communications"][i]["from"]["id"] = key.fromNode();
        outputEntity(j["communications"][i]["to"], key.toObj());
        break;
      }
      case elm::CommCategory::CollectionToNode:
      case elm::CommCategory::CollectionToNodeBcast: {
        if (key.cat_ == elm::CommCategory::CollectionToNode) {
          j["communications"][i]["type"] = "CollectionToNode";
        } else {
          j["communications"][i]["type"] = "CollectionToNodeBcast";
        }

        j["communications"][i]["to"]["type"] = "node";
        j["communications"][i]["to"]["id"] = key.toNode();
        outputEntity(j["communications"][i]["from"], key.fromObj());
        break;
      }
      case elm::CommCategory::ReadOnlyShared:
      case elm::CommCategory::WriteShared: {
        j["communications"][i]["type"] =
          (key.cat_ == elm::CommCategory::ReadOnlyShared) ?
          "ReadOnlyShared" : "WriteShared";
        j["communications"][i]["to"]["type"] = "node";
        j["communications"][i]["to"]["id"] = key.toNode();
        j["communications"][i]["from"]["type"] = "shared_id";
        j["communications"][i]["from"]["id"] = key.sharedID();
        break;
      }
      case elm::CommCategory::LocalInvoke:
      case elm::CommCategory::CollectiveToCollectionBcast:
        // not currently supported
        break;
      }
      i++;
    }
  }

  if (user_per_phase_json_.find(phase) != user_per_phase_json_.end()) {
    auto& user_def_this_phase = user_per_phase_json_.at(phase);

    if (!user_def_this_phase->empty()) {
      j["user_defined"] = *user_def_this_phase;
    }
  }

  return std::make_unique<json>(std::move(j));
}

#if vt_check_enabled(tv)
std::unique_ptr<vt::tv::PhaseWork> LBDataHolder::toTV(PhaseType phase) const {
  using vt::tv::PhaseWork;
  using vt::tv::ObjectWork;
  using vt::tv::ObjectCommunicator;

  std::unordered_map<ElementIDType, ObjectWork> objects;

  if (node_data_.find(phase) != node_data_.end()) {
    for (auto&& elm : node_data_.at(phase)) {
      ElementIDStruct id = elm.first;
      double whole_phase_load = elm.second.whole_phase_load;
      auto const& subphase_loads = elm.second.subphase_loads;

      ElmUserDataType user_defined;
      if (
        user_defined_lb_info_.find(phase) != user_defined_lb_info_.end() and
        user_defined_lb_info_.at(phase).find(id) !=
        user_defined_lb_info_.at(phase).end()
      ) {
        user_defined = user_defined_lb_info_.at(phase).at(id);
      }
      std::unordered_map<SubphaseType, double> subphase_map;
      for (std::size_t i = 0; i < subphase_loads.size(); i++) {
        subphase_map[i] = subphase_loads[i];
      }
      objects.try_emplace(
        id.id,
        // add id into map and then construct ObjectWork with these parameters
        ObjectWork(
          id.id, whole_phase_load, std::move(subphase_map), std::move(user_defined)
        )
      );
    }
  }

  if (node_comm_.find(phase) != node_comm_.end()) {
    for (auto&& elm : node_comm_.at(phase)) {
      auto const& key = elm.first;
      auto const& volume = elm.second;
      auto const& bytes = volume.bytes;
      switch(key.cat_) {
      case elm::CommCategory::SendRecv: {
        auto from_id = key.fromObj();
        auto to_id = key.toObj();

        if (objects.find(from_id.id) != objects.end()) {
          objects.at(from_id.id).addSentCommunications(to_id.id, bytes);
        } else if (objects.find(to_id.id) != objects.end()) {
          objects.at(to_id.id).addReceivedCommunications(from_id.id, bytes);
        }
        break;
      }
      default:
        // skip all other communications for now
        break;
      }
    }
  }

  return std::make_unique<PhaseWork>(phase, objects);
}

std::unordered_map<ElementIDType, tv::ObjectInfo> LBDataHolder::getObjInfo(
  PhaseType phase
) const {
  std::unordered_map<ElementIDType, tv::ObjectInfo> map;
  if (node_data_.find(phase) != node_data_.end()) {
    for (auto&& elm : node_data_.at(phase)) {
      ElementIDStruct id = elm.first;

      bool is_collection = false;
      bool is_objgroup = false;

      std::vector<uint64_t> idx;
      if (auto it = node_idx_.find(id); it != node_idx_.end()) {
        is_collection = true;
        idx = std::get<1>(it->second);
      }

      if (node_objgroup_.find(id) != node_objgroup_.end()) {
        is_objgroup = true;
      }

      tv::ObjectInfo oi{
        id.id, id.getHomeNode(), id.isMigratable(), std::move(idx)
      };
      oi.setIsCollection(is_collection);
      oi.setIsObjGroup(is_objgroup);
      map[id.id] = std::move(oi);
    }
  }
  return map;
}

#endif

LBDataHolder::LBDataHolder(nlohmann::json const& j)
{
  this_node_ = theContext()->getNode();

  // read metadata for skipped and identical phases
  readMetadata(j);

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
          auto home = task["entity"]["home"];
          bool is_migratable = task["entity"]["migratable"];

          vtAssertExpr(time.is_number());
          vtAssertExpr(node.is_number());

          if (etype == "object") {
            nlohmann::json object;
            bool is_bitpacked, is_collection;
            getObjectFromJsonField_(task["entity"], object, is_bitpacked, is_collection);

            // Create elm
            ElementIDStruct elm = is_collection and not is_bitpacked
              ? elm::ElmIDBits::createCollectionImpl(
                  is_migratable, static_cast<ElementIDType>(object), home, this_node_)
              : ElementIDStruct{object, this_node_};
            this->node_data_[id][elm].whole_phase_load = time;

            if (is_collection) {
              auto cid = task["entity"]["collection_id"];
              if (task["entity"].find("index") != task["entity"].end()) {
                auto idx = task["entity"]["index"];
                if (cid.is_number() && idx.is_array()) {
                  std::vector<uint64_t> arr = idx;
                  auto proxy = static_cast<VirtualProxyType>(cid);
                  this->node_idx_[elm] = std::make_tuple(proxy, arr);
                }
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

                  this->node_data_[id][elm].subphase_loads.resize(
                    static_cast<std::size_t>(sid) + 1);
                  this->node_data_[id][elm].subphase_loads[sid] = stime;
                }
              }
            }

            if (task.find("user_defined") != task.end()) {
              for (auto const& [key, value] : task["user_defined"].items()) {
                if (value.is_string()) {
                  user_defined_lb_info_[id][elm][key] =
                    value.template get<std::string>();
                }
                if (value.is_number()) {
                  user_defined_lb_info_[id][elm][key] =
                    value.template get<double>();
                }
              }
            }

            if (task.find("attributes") != task.end()) {
              for (auto const& [key, value] : task["attributes"].items()) {
                if (value.is_number_integer()) {
                  node_user_attributes_[id][elm][key] = value.get<int>();
                } else if (value.is_number_float()) {
                  node_user_attributes_[id][elm][key] = value.get<double>();
                } else if (value.is_string()) {
                  node_user_attributes_[id][elm][key] = value.get<std::string>();
                }
              }
            }
          }
        }
      }

      using CommKey = elm::CommKey;
      using CommVolume = elm::CommVolume;

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

              auto from_elm = getElmFromCommObject_(comm["from"]);
              auto to_elm = getElmFromCommObject_(comm["to"]);

              CommKey key(
                CommKey::CollectionTag{},
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

              auto to_elm = getElmFromCommObject_(comm["to"]);

              CommKey key(
                CommKey::NodeToCollectionTag{},
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

              auto from_elm = getElmFromCommObject_(comm["from"]);

              auto to_node = comm["to"]["id"];
              vtAssertExpr(to_node.is_number());

              CommKey key(
                CommKey::CollectionToNodeTag{},
                from_elm, static_cast<NodeType>(to_node),
                type == "CollectionToNodeBcast"
              );
              CommVolume vol{bytes, messages};
              this->node_comm_[id][key] = vol;
            } else if (
              type == "ReadOnlyShared" or type == "WriteShared"
            ) {
              vtAssertExpr(comm["from"]["type"] == "shared_id");
              vtAssertExpr(comm["to"]["type"] == "node");

              CommVolume vol{bytes, messages};
              auto to_node = comm["to"]["id"];
              vtAssertExpr(to_node.is_number());

              auto from_shared_id = comm["from"]["id"];
              vtAssertExpr(from_shared_id.is_number());

              if (type == "ReadOnlyShared") {
                CommKey key(
                  CommKey::ReadOnlySharedTag{},
                  static_cast<NodeType>(to_node),
                  static_cast<int>(from_shared_id)
                );
                this->node_comm_[id][key] = vol;
              } else {
                CommKey key(
                  CommKey::WriteSharedTag{},
                  static_cast<NodeType>(to_node),
                  static_cast<int>(from_shared_id)
                );
                this->node_comm_[id][key] = vol;
              }
            }
          }
        }
      }

      if (phase.find("user_defined") != phase.end()) {
        auto userDefined = phase["user_defined"];
        user_per_phase_json_[phase] = std::make_shared<nlohmann::json>();
        *(user_per_phase_json_[phase]) = userDefined;
      }
    }
  }

  // @todo: implement subphase communication de-serialization, no use for it
  // right now, so it will be ignored
}

void LBDataHolder::readMetadata(nlohmann::json const& j) {
  if (j.find("metadata") != j.end()) {
    auto metadata = j["metadata"];
    if (metadata.find("phases") != metadata.end()) {
      auto phases = metadata["phases"];
      // load all skipped phases
      auto sl = phases["skipped"]["list"];
      if(sl.is_array()) {
        for(PhaseType skipped : sl) {
          skipped_phases_.insert(skipped);
        }
      }
      auto sr = phases["skipped"]["range"];
      if(sr.is_array()) {
        for(auto const& pair : sr) {
          vtAssertExpr(pair.is_array());
          for(PhaseType i = pair[0]; i <= pair[1]; i++){
            skipped_phases_.insert(i);
          }
        }
      }
      // load all identical phases
      auto il = phases["identical_to_previous"]["list"];
      if(il.is_array()) {
        for(PhaseType identical : il) {
          identical_phases_.insert(identical);
        }
      }
      auto ir = phases["identical_to_previous"]["range"];
      if(ir.is_array()) {
        for(auto const& pair : ir) {
          vtAssertExpr(pair.is_array());
          for(PhaseType i = pair[0]; i <= pair[1]; i++){
            identical_phases_.insert(i);
          }
        }
      }
    }
    // load rank user atrributes
    if (metadata.find("attributes") != metadata.end()) {
      for (auto const& [key, value] : metadata["attributes"].items()) {
        if (value.is_number_integer()) {
          rank_attributes_[key] = value.get<int>();
        } else if (value.is_number_float()) {
          rank_attributes_[key] = value.get<double>();
        } else if (value.is_string()) {
          rank_attributes_[key] = value.get<std::string>();
        }
      }
    }
  }
}

void LBDataHolder::clear() {
  node_comm_.clear();
  node_data_.clear();
  node_subphase_comm_.clear();
  node_idx_.clear();
  skipped_phases_.clear();
  identical_phases_.clear();
}

}}}} /* end namespace vt::vrt::collection::balance */
