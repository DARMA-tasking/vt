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
#include <vt/utils/json/json_appender.h>

#include <string>
#include <vector>
#include <numeric>
#include <memory>

using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;
using ElementIDStruct = vt::vrt::collection::balance::ElementIDStruct;
using LoadSummary = vt::vrt::collection::balance::LoadSummary;

namespace vt {

bool split_more_blocks = true;

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
std::unordered_map<SharedID, int> shared_mem_initial_rank;
std::size_t max_shared_ids = 0;

std::tuple<double, int> calculateMemoryForRank(NodeType rank) {
  auto& r = ranks[rank];
  double total = 0;
  total += r.rank_working_bytes_;
  double max_working = 0;
  for (auto&& o : objects) {
    //auto& elm = o.first;
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

auto comp_rank_max = [](Rank* r1, Rank* r2) {
  return r1->cur_load_ < r2->cur_load_;
};
auto comp_rank_min = [](Rank* r1, Rank* r2) {
  return r1->cur_load_ > r2->cur_load_;
};

bool tryBin(
  Rank* max_rank, int bin, double size, int gid,
  std::unordered_map<SharedID, std::vector<ElementIDStruct>>& objs
);

bool tryBinFully(
  Rank* max_rank, int bin, double size, int gid,
  std::unordered_map<SharedID, std::vector<ElementIDStruct>>& objs
);

bool considerSwaps(
  Rank* max_rank, int bin, double size, int gid, double diff,
  std::unordered_map<SharedID, std::vector<ElementIDStruct>>& objs
);

void balanceLoad() {
  int made_no_assignments = 0;

  do {
#if 0
    for (auto&& r : ranks) {
      auto const rank = r.rank_;
      fmt::print(
        "rank={}: total_load={}, rank_working_bytes={}\n",
        rank, r.cur_load_, std::get<0>(calculateMemoryForRank(rank))
      );
    }
#endif

    std::vector<Rank*> max_ranks, min_ranks;
    for (auto& r : ranks) {
      max_ranks.push_back(&r);
    }

    std::make_heap(max_ranks.begin(), max_ranks.end(), comp_rank_max);
    std::pop_heap(max_ranks.begin(), max_ranks.end(), comp_rank_max);
    Rank* max_rank = max_ranks.back();
    max_ranks.pop_back();

    auto stats = computeStats();
    fmt::print(
      "avg={}, max={}, I={}, max_rank={} load={}\n",
      stats.avg_load_, stats.max_load_, stats.I(),
      max_rank->rank_, max_rank->cur_load_
    );

    auto diff = max_rank->cur_load_ - stats.avg_load_;
    fmt::print("diff={}\n", diff);

    std::map<SharedID, double> shared_map;
    std::unordered_map<SharedID, std::vector<ElementIDStruct>> obj_shared_map;
    std::vector<std::tuple<double, SharedID>> groupings;
    std::vector<double> groupings_sum;

    for (auto&& o : max_rank->objs_) {
      //fmt::print("object load={}, shared_id={}\n", o->load_, o->shared_id_);
      shared_map[o->shared_id_] += o->load_;
      obj_shared_map[o->shared_id_].push_back(o->elm_);
    }

    for (auto& x : obj_shared_map) {
      std::sort(
        x.second.begin(), x.second.end(),
        [](ElementIDStruct const& e1, ElementIDStruct const& e2) -> bool {
          return objects[e1].load_ > objects[e2].load_;
        }
      );
    }

    for (auto&& sm : shared_map) {
      fmt::print("id={}: load={}\n", sm.first, sm.second);
      groupings.push_back(std::make_tuple(sm.second,sm.first));
    }
    std::sort(
      groupings.begin(), groupings.end(),
      [](std::tuple<double,int> const& a, std::tuple<double,int> const& b) {
        return std::get<0>(a) < std::get<0>(b);
      }
    );
    groupings_sum.resize(groupings.size());
    groupings_sum[0] = std::get<0>(groupings[0]);
    int num = static_cast<int>(groupings.size());
    for (int i = 1; i < num; i++) {
      groupings_sum[i] = groupings_sum[i-1] + std::get<0>(groupings[i]);
    }
    for (auto&& g : groupings_sum) {
      fmt::print("grouping sum={}\n", g);
    }
    int pick_upper = 0;
    while (groupings_sum[pick_upper] < diff) pick_upper++;
    if (groupings_sum[pick_upper-1] >= diff * 1.05f) pick_upper--;
    int pick_lower = pick_upper-1;
    while (groupings_sum[pick_upper] - groupings_sum[pick_lower] < diff) pick_lower--;
    fmt::print("pick=({},{}]\n", pick_lower, pick_upper);

    bool made_assignment = false;

    if (made_no_assignments > 0 and split_more_blocks) {
      for (int i = 0; i < static_cast<int>(groupings.size()); i++) {
        auto size = std::get<0>(groupings[i]);
        auto gid = std::get<1>(groupings[i]);

        bool ret = considerSwaps(max_rank, i, size, gid, diff, obj_shared_map);
        made_assignment = made_assignment or ret;
        if (ret) break;
      }
    } else {
      for (int i = pick_lower+1; i < pick_upper+1; i++) {
        auto size = std::get<0>(groupings[i]);
        auto gid = std::get<1>(groupings[i]);

        bool ret = tryBin(max_rank, i, size, gid, obj_shared_map);
        made_assignment = made_assignment or ret;
      }
    }

    max_ranks.push_back(max_rank);
    std::push_heap(max_ranks.begin(), max_ranks.end(), comp_rank_max);

    if (not made_assignment) {
      made_no_assignments++;
    }

  } while (made_no_assignments < 3);

#if 0
  for (int i = 0; i < static_cast<int>(ranks.size()); i++) {
    std::tuple<double, int> total_mem = calculateMemoryForRank(i);
    fmt::print(
      "rank={}: total_memory={} B {} MiB, num shared blocks={}\n",
      i, std::get<0>(total_mem), std::get<0>(total_mem)/1024/1024, std::get<1>(total_mem)
    );
  }
#endif

#if 1
  std::set<SharedID> off_home;
  std::unordered_map<SharedID, std::set<int>> counter;
  for (auto&& r : ranks) {
    for (auto&& s : r.shared_ids_) {
      auto id = s.first;
      auto rank = r.rank_;
      counter[id].insert(rank);
      if (rank != shared_mem_initial_rank[id]) {
        off_home.insert(id);
      }
    }
  }

  int count_off_home = 0;
  for (auto&& c : counter) {
    if (c.second.size() == 1 and off_home.find(c.first) != off_home.end()) {
      count_off_home++;
    }
  }
  fmt::print("Histogram: count off home={}\n", count_off_home);

  std::map<int, int> histogram;
  for (auto&& c : counter) {
    //fmt::print("shared id={}, num ranks={}\n", c.first, c.second.size());
    histogram[c.second.size()]++;
  }

  for (auto&& h : histogram) {
    fmt::print("Histogram: bin={}, occur={}\n", h.first, h.second);
  }

  for (int i = 0; i < static_cast<int>(ranks.size()); i++) {
    auto& r = ranks[i];
    std::set<int> local;
    std::set<int> remote;
    std::set<int> split;
    for (auto&& x : r.shared_ids_) {
      auto id = x.first;
      if (counter[id].size() != 1) {
        split.insert(id);
      } else if (shared_mem_initial_rank[id] != r.rank_) {
        remote.insert(id);
      } else {
        local.insert(id);
      }
    }
    fmt::print("Rank {}: count={}: local=[", r.rank_, r.shared_ids_.size());
    for (auto&& x : local) {
      fmt::print("{} ", x);
    }
    fmt::print("] remote: [");
    for (auto&& x : remote) {
      fmt::print("{} ", x);
    }
    fmt::print("] split: [");
    for (auto&& x : split) {
      fmt::print("{}(home={}) ", x, shared_mem_initial_rank[x]);
    }
    fmt::print("]\n");
  }
#endif
}

bool tryBin(
  Rank* max_rank, int bin, double size, int gid,
  std::unordered_map<SharedID, std::vector<ElementIDStruct>>& objs
) {
  bool made_assignment = false;

  std::vector<Rank*> min_ranks;
  for (auto& r : ranks) {
    if (r.hasSharedID(gid) or r.shared_ids_.size() < max_shared_ids) {
      min_ranks.push_back(&r);
    }
  }
  std::make_heap(min_ranks.begin(), min_ranks.end(), comp_rank_min);

  Rank* min_rank = nullptr;

  int tally_assigned = 0;
  int tally_rejected = 0;

  for (auto&& o : objs[gid]) {
    if (min_ranks.size() == 0) {
      fmt::print("reached condition where no ranks could take the element\n");
      break;
    }

    if (min_rank == nullptr) {
      std::pop_heap(min_ranks.begin(), min_ranks.end(), comp_rank_min);
      min_rank = min_ranks.back();
      min_ranks.pop_back();
    }

    if (min_rank->shared_ids_.size() >= max_shared_ids and not min_rank->hasSharedID(gid)) {
      std::pop_heap(min_ranks.begin(), min_ranks.end(), comp_rank_min);
      min_rank = min_ranks.back();
      min_ranks.pop_back();
    }

    //fmt::print("min_rank={}, load={}, shared_ids={}\n", min_rank->rank_, min_rank->cur_load_, min_rank->shared_ids_.size());

    auto const selected_load = objects[o].load_;
    if (
      (min_rank->hasSharedID(gid) or min_rank->shared_ids_.size() < max_shared_ids) and
      min_rank->cur_load_ + selected_load < max_rank->cur_load_
    ) {
      //reassign object
      Object* obj_ptr = &objects[o];
      max_rank->removeObj(obj_ptr);
      min_rank->addObj(obj_ptr);
      made_assignment = true;

      tally_assigned++;

#if 0
      fmt::print("id={}: load={}: reassign to {}\n", o, objects[o].load_, min_rank->rank_);
#endif
    } else {

      if (min_rank->shared_ids_.size() >= max_shared_ids and not min_rank->hasSharedID(gid)) {
        // don't put it back on the heap
      } else {
        min_ranks.push_back(min_rank);
        std::push_heap(min_ranks.begin(), min_ranks.end(), comp_rank_min);
      }

      tally_rejected++;

#if 0
      fmt::print(
        "id={}: load={}: skipping; rank={}, has_id={}, size={}, new load={}\n",
        o, objects[o].load_, min_rank->rank_, min_rank->hasSharedID(gid),
        min_rank->shared_ids_.size(), min_rank->cur_load_ + selected_load
      );
#endif
    }
  }

  fmt::print(
    "try bin: {}, size={}, id={}; assigned={}, rejected={}\n",
    bin, size, gid, tally_assigned, tally_rejected
  );

  return made_assignment;
}

void outputBinInfo(Rank* r1) {
  std::unordered_map<SharedID, std::set<Object*>> binned;
  for (auto&& o : r1->objs_) {
    binned[o->shared_id_].insert(o);
  }
  for (auto&& bin : binned) {
    auto b = bin.first;
    auto& objs = bin.second;
    double load_sum = 0.;
    for (auto x : objs) {
      load_sum += x->load_;
    }
    fmt::print("Rank {}: bin={}, total load={}\n", r1->rank_, b, load_sum);
  }
}

bool considerSwaps(
  Rank* max_rank, int bin, double size, int gid, double diff,
  std::unordered_map<SharedID, std::vector<ElementIDStruct>>& objs
) {
  bool made_assignment = false;

  // fmt::print("considerSwaps: bin={}, size={}, diff={}\n", bin, size, diff);

  if (size > diff*0.3/* and size < diff*1.1*/) {
    // fmt::print("considerSwaps (continuing): bin={}, size={}, diff={}\n", bin, size, diff);
  } else {
    return false;
  }

  std::vector<Rank*> min_ranks;
  for (auto& r : ranks) {
    min_ranks.push_back(&r);
  }

  while (min_ranks.size() > 0) {
    Rank* min_rank = nullptr;

    std::pop_heap(min_ranks.begin(), min_ranks.end(), comp_rank_min);
    min_rank = min_ranks.back();
    min_ranks.pop_back();

    if (min_rank->rank_ == max_rank->rank_) {
      continue;
    }

    std::map<SharedID, std::set<Object*>> binned;
    for (auto&& o : min_rank->objs_) {
      binned[o->shared_id_].insert(o);
    }
    SharedID pick = -1;
    for (auto&& y : binned) {
      auto b = y.first;
      auto& os = y.second;
      double load_sum = 0.;
      for (auto&& x : os) {
        load_sum += x->load_;
      }

      double const cur_max = max_rank->cur_load_;
      if (min_rank->cur_load_ + size - load_sum < cur_max and max_rank->cur_load_ - size + load_sum < cur_max) {
        // fmt::print(
        //   "considerSwaps: continue testing: cur_max={}, new min={}, new max={}\n",
        //   cur_max,
        //   min_rank->cur_load_ + size - load_sum,
        //   max_rank->cur_load_ - size + load_sum
        // );
      } else {
        // fmt::print(
        //   "considerSwaps: would make situation worse: cur_max={}, new min={}, new max={}\n",
        //   cur_max,
        //   min_rank->cur_load_ + size - load_sum,
        //   max_rank->cur_load_ - size + load_sum
        // );
        continue;
      }

      // fmt::print("considerSwaps: min={} size={}, diff={}, bin={}, load_sum={}\n", min_rank->rank_, size, diff, b, load_sum);
      if (load_sum*1.1 < diff) {
        pick = b;
        break;
      }
    }

    if (pick != -1) {
      // fmt::print("considerSwaps: swapping pick={} rank={}\n", pick, min_rank->rank_);
      for (auto&& o : binned[pick]) {
        max_rank->addObj(o);
        min_rank->removeObj(o);
      }
      for (auto&& o : objs[gid]) {
        auto xx = &objects[o];
        min_rank->addObj(xx);
        max_rank->removeObj(xx);
      }
      // fmt::print("considerSwaps: min/max num ids= {} {}\n", min_rank->shared_ids_.size(), max_rank->shared_ids_.size());
      if (min_rank->shared_ids_.size() > max_shared_ids) {
        vtAbort("Failure\n");
      }
      if (max_rank->shared_ids_.size() > max_shared_ids) {
        vtAbort("Failure\n");
      }
      return true;
    }
  }

  return made_assignment;
}

bool tryBinFully(
  Rank* max_rank, int bin, double size, int gid,
  std::unordered_map<SharedID, std::vector<ElementIDStruct>>& objs
) {
  bool made_assignment = false;

  int tally_assigned = 0;
  int tally_rejected = 0;

  for (auto&& o : objs[gid]) {
    std::vector<Rank*> min_ranks;
    for (auto& r : ranks) {
      if (r.hasSharedID(gid) or r.shared_ids_.size() < max_shared_ids) {
        min_ranks.push_back(&r);
      }
    }
    std::make_heap(min_ranks.begin(), min_ranks.end(), comp_rank_min);

    while (min_ranks.size() > 0) {
      Rank* min_rank = nullptr;

      if (min_rank == nullptr) {
        std::pop_heap(min_ranks.begin(), min_ranks.end(), comp_rank_min);
        min_rank = min_ranks.back();
        min_ranks.pop_back();
      }

      if (min_rank->rank_ == max_rank->rank_) {
        continue;
      }

      //outputBinInfo(min_rank);
      //fmt::print("min_rank={}, load={}, shared_ids={}\n", min_rank->rank_, min_rank->cur_load_, min_rank->shared_ids_.size());

      auto const selected_load = objects[o].load_;
      if (
        (min_rank->hasSharedID(gid) or min_rank->shared_ids_.size() < max_shared_ids) and
        min_rank->cur_load_ + selected_load < max_rank->cur_load_
      ) {
        tally_assigned++;

        //reassign object
        Object* obj_ptr = &objects[o];
        max_rank->removeObj(obj_ptr);
        min_rank->addObj(obj_ptr);
        made_assignment = true;

#if 0
        fmt::print("id={}: load={}: reassign to {}\n", o, objects[o].load_, min_rank->rank_);
#endif
        break;
      } else {
        tally_rejected++;

#if 0
        fmt::print(
          "id={}: load={}: skipping; rank={}, has_id={}, size={}, new load={}\n",
          o, objects[o].load_, min_rank->rank_, min_rank->hasSharedID(gid),
          min_rank->shared_ids_.size(), min_rank->cur_load_ + selected_load
        );
#endif
      }
    }
  }

  fmt::print(
    "try bin fully: {}, size={}, id={}; assigned={}, rejected={}\n",
    bin, size, gid, tally_assigned, tally_rejected
  );

  return made_assignment;
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

void collateLBData(std::vector<LBDataHolder>& lb_data) {
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
        shared_mem_initial_rank[shared_id] = d.rank_;
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
    if (o.first.isMigratable()) {
      ranks[o.first.getHomeNode()].addObj(&o.second);
    }
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

void outputNewBalancedLoad(std::string const& name, std::vector<LBDataHolder>& lb_data) {
  for (int i = 0; i < static_cast<int>(ranks.size()); i++) {
    fmt::print("Outputting file for rank {}\n", i);
    LBDataHolder dh;
    auto& r = vt::ranks[i];
    for (auto&& o : r.objs_) {
      dh.node_data_[1][o->elm_] = LoadSummary{o->load_};
    }
    auto j2 = dh.toJson(1);
    auto j1 = lb_data[i].toJson(0);

    using JSONAppender = util::json::Appender<std::ofstream>;
    auto w = std::make_unique<JSONAppender>(
      "phases", "LBDatafile", fmt::format("{}.{}.json", name, r.rank_), false
    );
    w->addElm(*j1);
    w->addElm(*j2);
    w = nullptr;
  }
}

} /* end namespace vt */

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  std::vector<LBDataHolder> lb_data;

  double read_time = 0, collate_time = 0, lb_time = 0;

  vtAssert(argc == 2, "Must have one argument: \"x.%p.json\"");

  auto t1 = vt::timing::getCurrentTime();

  std::string file_pattern{argv[1]};

  int f = 0;
  bool found = true;
  std::size_t rank = file_pattern.find("%p");
  while (found) {
    std::string filename = file_pattern;
    auto str_rank = std::to_string(f);
    if (rank == std::string::npos) {
      filename = filename + str_rank;
    } else {
      filename.replace(rank, 2, str_rank);
    }

    std::fstream fs{filename};
    if (not fs.good()) {
      found = false;
      break;
    }

    fmt::print("Reading in filename={}\n", filename);
    auto p = vt::readInData(filename);
    lb_data.push_back(*p);
    f++;
  };

  read_time = vt::timing::getCurrentTime() - t1;

  t1 = vt::timing::getCurrentTime();
  vt::collateLBData(lb_data);
  collate_time = vt::timing::getCurrentTime() - t1;

  t1 = vt::timing::getCurrentTime();
  vt::balanceLoad();
  lb_time = vt::timing::getCurrentTime() - t1;

  auto stats = vt::computeStats();
  fmt::print(
    "Result: avg={}, max={}, I={:0.2f}, {:0.2f}s read, {:0.2f}s collate, {:0.4f}s LB\n",
    stats.avg_load_, stats.max_load_, stats.I(), read_time, collate_time, lb_time
  );

  bool output_new_distribution = true;

  if (output_new_distribution) {
    vt::outputNewBalancedLoad("out", lb_data);
  }

  vt::finalize();
  return 0;
}