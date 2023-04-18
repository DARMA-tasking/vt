/*
//@HEADER
// *****************************************************************************
//
//                               node_lb_data.cc
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

#include "vt/config.h"
#include "vt/vrt/collection/balance/node_lb_data.h"
#include "vt/vrt/collection/balance/baselb/baselb_msgs.h"
#include "vt/vrt/collection/manager.h"
#include "vt/timing/timing.h"
#include "vt/configs/arguments/app_config.h"
#include "vt/runtime/runtime.h"
#include "vt/utils/json/json_appender.h"
#include "vt/vrt/collection/balance/lb_data_holder.h"
#include "vt/elm/elm_lb_data.h"

#include <vector>
#include <unordered_map>
#include <cstdio>
#include <sys/stat.h>
#include <memory>

#include <fmt-vt/core.h>

namespace vt { namespace vrt { namespace collection { namespace balance {

void NodeLBData::setProxy(objgroup::proxy::Proxy<NodeLBData> in_proxy) {
  proxy_ = in_proxy;
}

/*static*/ std::unique_ptr<NodeLBData> NodeLBData::construct() {
  auto ptr = std::make_unique<NodeLBData>();
  auto proxy = theObjGroup()->makeCollective<NodeLBData>(
    ptr.get(), "NodeLBData"
  );
  proxy.get()->setProxy(proxy);
  return ptr;
}

bool NodeLBData::hasObjectToMigrate(ElementIDStruct obj_id) const {
  auto iter = node_migrate_.find(obj_id);
  return iter != node_migrate_.end();
}

bool NodeLBData::migrateObjTo(ElementIDStruct obj_id, NodeType to_node) {
  auto iter = node_migrate_.find(obj_id);
  if (iter == node_migrate_.end()) {
    return false;
  }

  auto migrate_fn = iter->second;
  migrate_fn(to_node);

  return true;
}

std::unordered_map<PhaseType, LoadMapType> const*
NodeLBData::getNodeLoad() const {
  return &lb_data_->node_data_;
}

std::unordered_map<PhaseType, CommMapType> const* NodeLBData::getNodeComm() const {
  return &lb_data_->node_comm_;
}

std::unordered_map<PhaseType, std::unordered_map<SubphaseType, CommMapType>> const* NodeLBData::getNodeSubphaseComm() const {
  return &lb_data_->node_subphase_comm_;
}

CommMapType* NodeLBData::getNodeComm(PhaseType phase) {
  auto iter = lb_data_->node_comm_.find(phase);
  return (iter != lb_data_->node_comm_.end()) ? &iter->second : nullptr;
}

void NodeLBData::clearLBData() {
  lb_data_->clear();
  node_migrate_.clear();
  next_elm_ = 1;
}

void NodeLBData::startIterCleanup(PhaseType phase, unsigned int look_back) {
  if (phase >= look_back) {
    lb_data_->node_data_.erase(phase - look_back);
    lb_data_->node_comm_.erase(phase - look_back);
    lb_data_->node_subphase_comm_.erase(phase - look_back);
  }

  // Clear migrate lambdas and proxy lookup since LB is complete
  NodeLBData::node_migrate_.clear();
  node_collection_lookup_.clear();
  node_objgroup_lookup_.clear();
}

ElementIDType NodeLBData::getNextElm() {
  return next_elm_++;
}

void NodeLBData::initialize() {
  lb_data_ = std::make_unique<LBDataHolder>();

#if vt_check_enabled(lblite)
  if (theConfig()->vt_lb_data) {
    theNodeLBData()->createLBDataFile();
  }
#endif
}

void NodeLBData::loadAndBroadcastSpec() {
  using namespace ::vt::utils::file_spec;

  if (theConfig()->vt_lb_spec) {
    auto spec_proxy = FileSpec::construct(FileSpecType::LB);

    theTerm()->produce();
    if (theContext()->getNode() == 0) {
      auto spec_ptr = spec_proxy.get();
      spec_ptr->parse();
      spec_ptr->broadcastSpec();
    }
    theSched()->runSchedulerWhile(
      [&spec_proxy] { return not spec_proxy.get()->specReceived(); });
    theTerm()->consume();

    spec_proxy_ = spec_proxy.getProxy();
  }
}

void NodeLBData::createLBDataFile() {
  auto const file_name = theConfig()->getLBDataFileOut();
  auto const compress = theConfig()->vt_lb_data_compress;

  vt_debug_print(
    normal, lb,
    "NodeLBData::createLBDataFile: file={}\n", file_name
  );

  auto const dir = theConfig()->vt_lb_data_dir;
  // Node 0 creates the directory
  if (not created_dir_ and theContext()->getNode() == 0) {
    int flag = mkdir(dir.c_str(), S_IRWXU);
    if (flag < 0 && errno != EEXIST) {
      throw std::runtime_error("Failed to create directory: " + dir);
    }
    created_dir_ = true;
  }

  // Barrier: wait for node 0 to create directory before trying to put a file in
  // the LB data destination directory
  if (curRT) {
    curRT->systemSync();
  } else {
    // Something is wrong
    vtAssert(false, "Trying to dump LB data when VT runtime is deallocated?");
  }

  using JSONAppender = util::json::Appender<std::ofstream>;

  if (not lb_data_writer_) {
    nlohmann::json metadata;
    if (curRT->has_physical_node_info) {
      nlohmann::json node_metadata;
      node_metadata["id"] = curRT->physical_node_id;
      node_metadata["size"] = curRT->physical_node_size;
      node_metadata["rank"] = curRT->physical_node_rank;
      node_metadata["num_nodes"] = curRT->physical_num_nodes;
      metadata["shared_node"] = node_metadata;
    }
    metadata["type"] = "LBDatafile";
    metadata["rank"] = theContext()->getNode();
    auto phasesMetadata = lb_data_->metadataToJson();
    if(phasesMetadata) {
       metadata["phases"] = *phasesMetadata;
    }
    lb_data_writer_ = std::make_unique<JSONAppender>(
      "phases", metadata, file_name, compress
    );
  }
}

void NodeLBData::finalize() {
  closeLBDataFile();

  // If LB data are enabled, close output file and clear LB data
#if vt_check_enabled(lblite)
  if (theConfig()->vt_lb_data) {
    clearLBData();
  }
#endif
}

void NodeLBData::fatalError() {
  // make flush occur on all LB data collected immediately
  closeLBDataFile();
}

void NodeLBData::closeLBDataFile() {
  lb_data_writer_ = nullptr;
}

std::pair<ElementIDType, ElementIDType>
getRecvSendDirection(elm::CommKeyType const& comm) {
  switch (comm.cat_) {
  case elm::CommCategory::SendRecv:
  case elm::CommCategory::Broadcast:
    return std::make_pair(comm.toObj().id, comm.fromObj().id);

  case elm::CommCategory::NodeToCollection:
  case elm::CommCategory::NodeToCollectionBcast:
    return std::make_pair(comm.toObj().id, comm.fromNode());

  case elm::CommCategory::CollectionToNode:
  case elm::CommCategory::CollectionToNodeBcast:
    return std::make_pair(comm.toNode(), comm.fromObj().id);

  // Comm LB data are not recorded for local operations
  // this case is just to avoid warning of not handled enum
  case elm::CommCategory::CollectiveToCollectionBcast:
  case elm::CommCategory::LocalInvoke:
    return std::make_pair(ElementIDType{}, ElementIDType{});
  }

  vtAssert(false, "Invalid balance::CommCategory enum value");
  return std::make_pair(ElementIDType{}, ElementIDType{});
}

void NodeLBData::outputLBDataForPhase(PhaseType phase) {
  bool enabled_for_phase = true;
  if(spec_proxy_ != vt::no_obj_group){
    vt::objgroup::proxy::Proxy<utils::file_spec::FileSpec> proxy(spec_proxy_);
    enabled_for_phase = proxy.get()->checkEnabled(phase);
  }

  // LB data output when LB is enabled and appropriate flag is enabled
  if (!theConfig()->vt_lb_data or not enabled_for_phase) {
    return;
  }

  vt_print(lb, "NodeLBData::outputLBDataForPhase: phase={}\n", phase);

  using JSONAppender = util::json::Appender<std::ofstream>;

  auto j = lb_data_->toJson(phase);
  auto writer = static_cast<JSONAppender*>(lb_data_writer_.get());
  writer->addElm(*j);
}

void NodeLBData::registerCollectionInfo(
  ElementIDStruct id, VirtualProxyType proxy,
  std::vector<uint64_t> const& index, MigrateFnType migrate_fn
) {
  // Add the index to the map
  lb_data_->node_idx_[id] = std::make_tuple(proxy, index);
  node_migrate_[id] = migrate_fn;
  node_collection_lookup_[id] = proxy;
}

void NodeLBData::registerObjGroupInfo(
  ElementIDStruct id, ObjGroupProxyType proxy
) {
  lb_data_->node_objgroup_[id] = proxy;
  node_objgroup_lookup_[id] = proxy;
}

void NodeLBData::addNodeLBData(
  ElementIDStruct id, elm::ElementLBData* in, StorableType *storable,
  SubphaseType focused_subphase
) {
  vt_debug_print(
    normal, lb,
    "NodeLBData::addNodeLBData: id={}\n", id
  );

  auto const phase = in->getPhase();
  auto const& total_load = in->getLoad(phase, focused_subphase);

  auto &phase_data = lb_data_->node_data_[phase];
  auto elm_iter = phase_data.find(id);
  vtAssert(elm_iter == phase_data.end(), "Must not exist");

  auto& subphase_times = in->getSubphaseTimes(phase);

  phase_data.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(LoadSummary{total_load, subphase_times})
  );

  auto const& comm = in->getComm(phase);
  auto &comm_data = lb_data_->node_comm_[phase];
  for (auto&& c : comm) {
    comm_data[c.first] += c.second;
  }

  auto const& subphase_comm = in->getSubphaseComm(phase);
  auto &subphase_comm_data = lb_data_->node_subphase_comm_[phase];
  for (SubphaseType i = 0; i < subphase_comm.size(); i++) {
    for (auto& sp : subphase_comm[i]) {
      subphase_comm_data[i][sp.first] += sp.second;
    }
  }

  if (storable) {
    lb_data_->user_defined_json_[phase][id] = std::make_shared<nlohmann::json>(
      storable->toJson()
    );
  }

  in->updatePhase(1);

  auto model = theLBManager()->getLoadModel();
  in->releaseLBDataFromUnneededPhases(phase, model->getNumPastPhasesNeeded());
}

VirtualProxyType NodeLBData::getCollectionProxyForElement(
  ElementIDStruct obj_id
) const {
  auto iter = node_collection_lookup_.find(obj_id);
  if (iter == node_collection_lookup_.end()) {
    return no_vrt_proxy;
  }
  return iter->second;
}

ObjGroupProxyType NodeLBData::getObjGroupProxyForElement(
  ElementIDStruct obj_id
) const {
  auto iter = node_objgroup_lookup_.find(obj_id);
  if (iter == node_objgroup_lookup_.end()) {
    return no_obj_group;
  }
  return iter->second;
}

}}}} /* end namespace vt::vrt::collection::balance */
