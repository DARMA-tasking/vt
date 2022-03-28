/*
//@HEADER
// *****************************************************************************
//
//                         predictive_model_dumper.cc
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

#include <vt/transport.h>
#include <vt/vrt/collection/balance/lb_common.h>
#include <vt/vrt/collection/balance/stats_data.h>
#include <vt/utils/json/json_appender.h>

// This example is related to modeling task loads for an application
// that does not obey the principle of persistence.  An underlying
// assumption is that each task is executed only once in the lifetime
// of the application.  Chacteristics of the task can be used to
// predict the task load.  This example reads in a file with one task
// per line, with each line containing:
//   home_rank measured_load model-predicted_load serialized_bytes return_bytes
// This driver will dump a JSON file where all tasks appear to execute
// on their home ranks as listed.  Phase 0 lists model-predicted loads
// while phase 1 lists measured loads.

// The JSON file can be fed into the workload replay driver.  Phase 0 of
// the execution will reflect the model, thus allowing the load
// balancer place tasks based on the model alone.  Phase 1 will
// reflect measured home-rank loads, but the workload replay driver will
// apply them at the post-LB locations, allowing for evaluating the
// effectiveness of the model for balancing the load.  This of course
// assumes that the loads are independent of execution location.

struct OneAndDoneCol : vt::Collection<OneAndDoneCol, vt::Index3D> {
  OneAndDoneCol() = default;

  inline static vt::NodeType collectionMap(
    vt::Index3D* idx, vt::Index3D*, vt::NodeType
  ) {
    return idx->x();
  }
};

struct RankManager {
  RankManager() {
    auto proxy = vt::theObjGroup()->makeCollective(this);
    proxy_ = proxy;
  }
  vt::objgroup::proxy::Proxy<RankManager> proxy_;
};

struct TaskData {
  double measured_load;
  double modeled_load;
  double serialized_bytes;
  double return_bytes;
};


int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  if (argc != 4) {
    vtAbort(
      "Specify the input and output file name, as well as a modeled load floor,"
      " on the command line.\n"
    );
  }

  vt::NodeType this_node = vt::theContext()->getNode();

  auto in_file_name = argv[1];
  std::string out_file_name(argv[2]);
  auto load_floor = atof(argv[3]);

  std::size_t rank = out_file_name.find("%p");
  auto str_rank = std::to_string(this_node);
  if (rank == std::string::npos) {
    out_file_name = out_file_name + str_rank;
  } else {
    out_file_name.replace(rank, 2, str_rank);
  }

  fmt::print(
    "{}: input: {}\n{}: output: {}\n",
    this_node, in_file_name, this_node, out_file_name
  );

  std::map<vt::Index3D, TaskData> tasks;
  std::map<int, int> count_this_block;

  std::ifstream f(in_file_name);
  std::string line;
  while (std::getline(f, line)) {
    std::istringstream iss(line);
    int mpi_rank = -1;
    double measured_load = 0.0;
    double modeled_load = 0.0;
    double serialized_bytes = 0;
    double return_bytes = 0;
    iss >> mpi_rank >> measured_load >> modeled_load
        >> serialized_bytes >> return_bytes;

    int mod_mpi_rank = mpi_rank;
    int overdecomp = 0;
    if (mod_mpi_rank == this_node) {
      vt::Index3D index{
        mod_mpi_rank, overdecomp, count_this_block[overdecomp]
      };
      tasks[index] = TaskData{
        measured_load, modeled_load, serialized_bytes, return_bytes
      };
      ++(count_this_block[overdecomp]);
    }
  }
  f.close();

  for (auto &block : count_this_block) {
    fmt::print(
      "{}: found {} tasks for rank={} block={}\n",
      this_node, block.second, this_node, block.first
    );
  }

  std::vector<std::tuple<vt::Index3D, std::unique_ptr<OneAndDoneCol>>> elms;
  for (auto &task : tasks) {
    elms.emplace_back(
      std::make_tuple(task.first, std::make_unique<OneAndDoneCol>())
    );
  }

  auto proxy = vt::makeCollection<OneAndDoneCol>()
    .listInsertHere(std::move(elms))
    .mapperFunc<OneAndDoneCol::collectionMap>()
    .wait();

  RankManager man;
  auto objgrp_id = vt::elm::ElmIDBits::createObjGroup(
    man.proxy_.getProxy(), this_node
  );

  vt::vrt::collection::balance::StatsData sd;

  using LoadMapType = vt::vrt::collection::balance::LoadMapType;
  using LoadSummary = vt::vrt::collection::balance::LoadSummary;
  using CommMapType = vt::vrt::collection::balance::CommMapType;
  using CommKey = vt::elm::CommKey;
  using CommVolume = vt::elm::CommVolume;
  using CommBytesType = vt::elm::CommBytesType;

  LoadMapType modeled_phase, measured_phase;
  CommMapType node_comm;

  for (auto it = tasks.begin(); it != tasks.end(); ++it) {
    vt::Index3D idx = it->first;
    auto elm_ptr = proxy(idx).tryGetLocalPtr();
    assert(elm_ptr != nullptr);
    auto elm_id = elm_ptr->getElmID();

    TaskData &t = it->second;
    modeled_phase[elm_id] = LoadSummary{std::max(t.modeled_load, load_floor)};
    measured_phase[elm_id] = LoadSummary{t.measured_load};

    CommKey skey(CommKey::SendRecvTag{}, objgrp_id, elm_id, false);
    CommVolume svol{static_cast<CommBytesType>(t.serialized_bytes), 1};
    node_comm[skey] = svol;

    CommKey rkey(CommKey::SendRecvTag{}, elm_id, objgrp_id, false);
    CommVolume rvol{static_cast<CommBytesType>(t.return_bytes), 1};
    node_comm[rkey] = rvol;

    std::vector<uint64_t> arr;
    arr.push_back(idx.x());
    arr.push_back(idx.y());
    arr.push_back(idx.z());
    sd.node_idx_[elm_id] = std::make_tuple(proxy.getProxy(), arr);
  }

  sd.node_data_[0] = modeled_phase;
  sd.node_comm_[0] = node_comm;

  sd.node_data_[1] = measured_phase;
  sd.node_comm_[1] = node_comm;

  // add the measured loads again so we can see what the LB could have been
  // with perfect load predictions
  sd.node_data_[2] = measured_phase;
  sd.node_comm_[2] = node_comm;

  auto const compress = false;
  using JSONAppender = vt::util::json::Appender<std::ofstream>;
  auto json_writer = std::make_unique<JSONAppender>(
    "phases", out_file_name, compress
  );
  {
    auto j = sd.toJson(0);
    json_writer->addElm(*j);
  }
  {
    auto k = sd.toJson(1);
    json_writer->addElm(*k);
  }
  {
    auto k = sd.toJson(2);
    json_writer->addElm(*k);
  }
  json_writer = nullptr;

  vt::finalize();
  return 0;
}
