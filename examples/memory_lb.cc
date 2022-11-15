/*
//@HEADER
// *****************************************************************************
//
//                                 memory_lb.cc
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
#include <vt/vrt/collection/balance/node_lb_data.h>
#include <vt/vrt/collection/balance/lb_data_holder.h>
#include <vt/utils/json/json_reader.h>

#include <string>
#include <vector>
#include <numeric>

using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;
using ElementIDStruct = vt::vrt::collection::balance::ElementIDStruct;

namespace vt {

using SharedID = int;

struct Object {
  Object() = default;
  Object(ElementIDStruct in_elm, double in_load)
    : elm_(in_elm),
      load_(in_load),
      assigned_rank_(elm_.getHomeNode())
  { }

  ElementIDStruct elm_;
  double load_ = 0;
  NodeType assigned_rank_ = uninitialized_destination;
  double footprint_bytes_ = 0;
  double working_bytes_ = 0;
  SharedID shared_id_ = -1;
};

struct Rank {
  Rank() = default;
  Rank(NodeType in_rank, double in_rank_working_bytes, double in_initial_load)
    : rank_(in_rank),
      rank_working_bytes_(in_rank_working_bytes),
      initial_load_(in_initial_load)
  { }

  void addObj(Object* obj) {
    objs_.insert(obj);
    shared_ids_[obj->shared_id_]++;
    cur_load_ += obj->load_;
    obj->assigned_rank_ = rank_;
  };

  void removeObj(Object* obj) {
    auto iter = objs_.find(obj);
    vtAssert(iter != objs_.end(), "Must exist");
    objs_.erase(iter);
    shared_ids_[obj->shared_id_]--;
    if (shared_ids_[obj->shared_id_] == 0) {
      shared_ids_.erase(shared_ids_.find(obj->shared_id_));
    }
    cur_load_ -= obj->load_;
  }

  bool hasSharedID(SharedID id) const {
    return shared_ids_.find(id) != shared_ids_.end();
  }

  NodeType rank_ = uninitialized_destination;
  double rank_working_bytes_ = 0;
  double initial_load_ = 0;
  double cur_load_ = 0;
  std::set<Object*> objs_;
  std::map<SharedID, int> shared_ids_;
};

std::unordered_map<ElementIDStruct, Object> objects;
std::vector<Rank> ranks;
std::unordered_map<SharedID, double> shared_mem;
std::size_t max_shared_ids = 0;

std::tuple<double, int> calculateMemoryForRank(NodeType rank) {
  auto& r = ranks[rank];
  double total = 0;
  total += r.rank_working_bytes_;
  double max_working = 0;
  for (auto&& o : objects) {
    auto& rec = o.second;
    if (rec.assigned_rank_ == rank) {
      max_working = std::max(max_working, rec.working_bytes_);
      total += rec.footprint_bytes_;
    }
  }
  total += max_working;
  for (auto&& id : r.shared_ids_) {
    total += shared_mem[id.first];
  }

  max_shared_ids = std::max(r.shared_ids_.size(), max_shared_ids);
  return std::make_tuple(total, static_cast<int>(r.shared_ids_.size()));
}

struct Stats {
  double avg_load_ = 0;
  double max_load_ = 0;
  double I() const { return (max_load_ / avg_load_) - 1.0; }
};

Stats computeStats() {
  Stats s;
  for (auto&& r : ranks) {
    s.max_load_ = std::max(r.cur_load_, s.max_load_);
    s.avg_load_ += r.cur_load_;
  }
  s.avg_load_ /= ranks.size();
  return s;
}

std::unique_ptr<LBDataHolder> readInData(std::string const& file_name) {
  using vt::util::json::Reader;
  Reader r{file_name};
  auto j = r.readFile();
  auto d = std::make_unique<LBDataHolder>(*j);
  // for (auto&& elm : d->user_defined_[0]) {
  //   for (auto&& user : elm.second) {
  //     fmt::print("elm={}: f={}, s={}\n", elm.first, user.first, user.second);
  //   }
  // }
  return d;
}

void balanceLoad() {
  // @todo implement
}

void collateLBData(std::vector<LBDataHolder> lb_data) {
  ranks.resize(lb_data.size()); //num ranks == lb_data.size()

  for (auto&& d : lb_data) {
    auto l_0 = d.node_data_[0];
    auto ud_0 = d.user_defined_[0];
    double total_load = 0;
    for (auto&& x : l_0) {
      auto const& elm = x.first;
      vtAssert(objects.find(elm) == objects.end(), "Must not exist");
      objects.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(elm),
        std::forward_as_tuple(elm, x.second.whole_phase_load)
      );
      total_load += x.second.whole_phase_load;
    }
    double rank_working_bytes = 0;
    for (auto&& x : ud_0) {
      auto const& elm = x.first;
      int shared_id = -1;
      double shared_bytes = 0;
      for (auto&& y : x.second) {
        if (y.first == "rank_working_bytes") {
          rank_working_bytes = y.second;
        }
        if (y.first == "shared_id") {
          shared_id = static_cast<int>(y.second);
          objects[elm].shared_id_ = shared_id;
        }
        if (y.first == "shared_bytes") {
          shared_bytes = y.second;
        }
        if (y.first == "task_footprint_bytes") {
          objects[elm].footprint_bytes_ = y.second;
        }
        if (y.first == "task_working_bytes") {
          objects[elm].working_bytes_ = y.second;
        }
      }
      if (shared_id != -1) {
        shared_mem[shared_id] = shared_bytes;
      }
    }
    auto const rank = d.rank_;
    ranks[rank] = Rank{rank, rank_working_bytes, total_load};
    fmt::print(
      "rank={}: total_load={}, rank_working_bytes={}\n",
      rank, total_load, rank_working_bytes
    );
  }

  for (auto&& o : objects) {
    ranks[o.first.getHomeNode()].addObj(&o.second);
  }

  for (int i = 0; i < static_cast<int>(ranks.size()); i++) {
    std::tuple<double, int> total_mem = calculateMemoryForRank(i);
    fmt::print(
      "rank={}: total_memory={} B {} MiB, num shared blocks={}\n",
      i, std::get<0>(total_mem), std::get<0>(total_mem)/1024/1024, std::get<1>(total_mem)
    );
  }

  max_shared_ids += 1;
  auto stats = computeStats();
  fmt::print(
    "avg={}, max={}, I={}, max_shared_ids={}\n",
    stats.avg_load_, stats.max_load_, stats.I(), max_shared_ids
  );

  for (auto&& sm : shared_mem) {
    fmt::print(
      "shared id={}, mem={} B {:0.2f} MiB\n",
      sm.first, sm.second, sm.second/1024/1024
    );
  }

  // for (auto&& o : objects) {
  //   fmt::print("elm={}, load={}\n", o.first, o.second.load_);
  // }
}

} /* end namespace vt */

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  std::vector<LBDataHolder> lb_data;

  double read_time = 0, collate_time = 0, lb_time = 0;

  auto t1 = vt::timing::getCurrentTime();
  for (int i = 1; i < argc; i++) {
    std::string const filename = std::string{argv[i]};
    fmt::print("Reading in filename={}\n", filename);
    auto p = vt::readInData(filename);
    lb_data.push_back(*p);
  }
  read_time = vt::timing::getCurrentTime() - t1;

  t1 = vt::timing::getCurrentTime();
  vt::collateLBData(std::move(lb_data));
  collate_time = vt::timing::getCurrentTime() - t1;

  t1 = vt::timing::getCurrentTime();
  vt::balanceLoad();
  lb_time = vt::timing::getCurrentTime() - t1;

  auto stats = vt::computeStats();
  fmt::print(
    "Result: avg={}, max={}, I={:0.2f}, {:0.2f}s read, {:0.2f}s collate, {:0.4f}s LB\n",
    stats.avg_load_, stats.max_load_, stats.I(), read_time, collate_time, lb_time
  );

  vt::finalize();
  return 0;
}
