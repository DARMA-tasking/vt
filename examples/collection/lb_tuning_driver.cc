/*
//@HEADER
// *****************************************************************************
//
//                            lb_tuning_driver.cc
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

#include <vt/transport.h>
#include <vt/vrt/collection/balance/model/per_collection.h>
#include <vt/vrt/collection/balance/model/composed_model.h>
#include <vt/vrt/collection/balance/model/load_model.h>

#include <memory>
#include <cinttypes>
#include <regex>

inline vt::NodeType elmIndexMap(vt::Index2D* idx, vt::Index2D*, vt::NodeType) {
  return idx->x();
}

struct MappingMsg : vt::Message {
  using ObjectIDType = vt::vrt::collection::balance::ElementIDType;

  vt::Index2D index_;
  ObjectIDType obj_id_;

  explicit MappingMsg(vt::Index2D index, ObjectIDType obj_id)
    : index_(index), obj_id_(obj_id)
  { }
};

inline vt::NodeType findDirectoryNode(MappingMsg::ObjectIDType obj_id) {
  auto const nranks = vt::theContext()->getNumNodes();
  return obj_id % nranks;
}

static std::unordered_map<MappingMsg::ObjectIDType, vt::Index2D> obj_id_mapping;

static void ReceiveMapping(MappingMsg* msg) {
  auto obj_id = msg->obj_id_;
  auto index = msg->index_;
  obj_id_mapping[obj_id] = index;
  vt_debug_print(
    gen, node,
    "ReceiveMapping: obj {} is index {}\n",
    obj_id, index
  );
}

struct PlaceHolder : vt::Collection<PlaceHolder, vt::Index2D> {
  using TestMsg = vt::CollectionMessage<PlaceHolder>;

  struct MigrateHereMsg : vt::CollectionMessage<PlaceHolder> {
    using MessageParentType = vt::CollectionMessage<PlaceHolder>;

    MigrateHereMsg() = default;

    MigrateHereMsg(vt::NodeType src)
      : src_(src)
    { }

    template <typename Serializer>
    void serialize(Serializer& s) {
      MessageParentType::serialize(s);
      s | src_;
    }

    vt::NodeType src_ = vt::uninitialized_destination;
  };

  struct StatsDataMsg : vt::CollectionMessage<PlaceHolder> {
    using MessageParentType = vt::CollectionMessage<PlaceHolder>;
    using ObjStatsMapType = std::unordered_map<
      int /*phase from stats file*/, vt::TimeType
    >;
    vt_msg_serialize_required();

    StatsDataMsg() = default;

    StatsDataMsg(const ObjStatsMapType &stats)
      : stats_(stats)
    { }

    template <typename Serializer>
    void serialize(Serializer& s) {
      MessageParentType::serialize(s);
      s | stats_;
    }

    ObjStatsMapType stats_;
  };

  void timestep(TestMsg* msg) { }

  void shareMapping(TestMsg* msg) {
    // announce mapping from perm id to index to rank determined by hashing
    auto obj_id = this->getElmID();
    auto index = this->getIndex();
    auto response = vt::makeMessage<MappingMsg>(index, obj_id.id);
    auto const this_rank = vt::theContext()->getNode();
    vt::NodeType dest = findDirectoryNode(obj_id.id);
    vt_debug_print(
      gen, node,
      "shareMapping: sharing with {} that obj {} is index {}\n",
      dest, obj_id.id, index
    );
    if (dest != this_rank) {
      vt::theMsg()->sendMsg<MappingMsg, ReceiveMapping>(dest, response);
    } else {
      ReceiveMapping(response.get());
    }
  }

  void migrateSelf(MigrateHereMsg* msg) {
    // migrate oneself to the requesting rank
    auto const this_rank = vt::theContext()->getNode();
    auto dest = msg->src_;
    if (dest != this_rank) {
      vt_debug_print(
        gen, node,
        "migrateSelf: index {} asked to migrate from {} to {}\n",
        this->getIndex(), this_rank, dest
      );
      this->migrate(dest);
    }
  }

  void recvStatsData(StatsDataMsg *msg) {
    vt_debug_print(
      gen, node,
      "recvStatsData: index {} received stats data for {} phases\n",
      this->getIndex(), msg->stats_.size()
    );
    stats_from_file_.insert(msg->stats_.begin(), msg->stats_.end());
  }

  vt::TimeType getLoad(int phase_in_stats_file) {
    return stats_from_file_[phase_in_stats_file];
  }

  template <typename Serializer>
  void serialize(Serializer& s) {
    vt::Collection<PlaceHolder, vt::Index2D>::serialize(s);
    s | stats_from_file_;
  }

  virtual void epiMigrateIn() {
    auto obj_id = this->getElmID();
    auto index = this->getIndex();
    obj_id_mapping[obj_id.id] = index;
  }

  // carry my loads from the stats files with me
  StatsDataMsg::ObjStatsMapType stats_from_file_;
};

struct QueryMsg : vt::Message {
  using ObjectIDType = MappingMsg::ObjectIDType;

  ObjectIDType obj_id_;
  vt::NodeType src_;

  explicit QueryMsg(ObjectIDType obj_id, vt::NodeType src)
    : obj_id_(obj_id), src_(src)
  { }
};

static void RequestMapping(QueryMsg *msg) {
  auto dest = msg->src_;
  auto obj_id = msg->obj_id_;
  auto iter = obj_id_mapping.find(obj_id);
  if (iter == obj_id_mapping.end()) {
    vt_debug_print(
      gen, node,
      "RequestMapping: {} asked for index of {} but it is unknown\n",
      dest, obj_id
    );
    vtAbort("unknown id");
  }

  auto idxiter = obj_id_mapping.find(obj_id);
  vtAssert(
    idxiter != obj_id_mapping.end(),
    "Element ID to index mapping must be known"
  );
  auto index = idxiter->second;
  auto response = vt::makeMessage<MappingMsg>(index, obj_id);
  vt_debug_print(
    gen, node,
    "RequestMapping: responding that obj {} is index {}\n",
    obj_id, index
  );
  vt::theMsg()->sendMsg<MappingMsg, ReceiveMapping>(dest, response);
}

using vt::vrt::collection::balance::ComposedModel;
using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::ElementIDStruct;
using vt::vrt::collection::balance::ElementIDType;
using vt::vrt::collection::balance::PhaseOffset;
using vt::vrt::collection::balance::PerCollection;

ElementIDType convertReleaseStatsID(ElementIDType release_perm_id) {
  auto local_id = release_perm_id >> 32;
  auto node_id = release_perm_id - (local_id << 32);
  auto converted_local_id = (local_id - 1) / 2;
  auto converted_elm_id = converted_local_id << 32 | node_id;
  return converted_elm_id;
}

struct FileModel : ComposedModel {
  using ProxyType = vt::CollectionProxy<PlaceHolder, vt::Index2D>;

  FileModel(
    std::shared_ptr<LoadModel> in_base, std::string const& filename,
    int initial_phase, int phases_to_run, int convert_from_release,
    ProxyType proxy
  )
    : vt::vrt::collection::balance::ComposedModel(in_base),
      initial_phase_(initial_phase), phases_to_run_(phases_to_run),
      convert_from_release_(convert_from_release), proxy_(proxy)
  {
    parseFile(filename);
  }

  vt::TimeType getWork(ElementIDStruct elmid, PhaseOffset offset) override {
    auto const phase = getNumCompletedPhases()-1 + initial_phase_;
    vt_debug_print(
      gen, node,
      "getWork {} phase={}\n",
      elmid.id, phase
    );

    vtAbortIf(
      offset.phases != PhaseOffset::NEXT_PHASE,
      "This driver only supports offset.phases == NEXT_PHASE"
    );
    vtAbortIf(
      offset.subphase != PhaseOffset::WHOLE_PHASE,
      "This driver only supports offset.subphase == WHOLE_PHASE"
    );

    auto idxiter = obj_id_mapping.find(elmid.id);
    vtAssert(
      idxiter != obj_id_mapping.end(),
      "Element ID to index mapping must be known"
    );
    auto index = idxiter->second;

    auto elm_ptr = proxy_(index).tryGetLocalPtr();
    if (elm_ptr == nullptr) {
      vt_debug_print(
        gen, node,
        "could not find elm_id={} index={}\n",
        elmid.id, index
      );
      return 0; // FIXME: this BREAKS the O_l statistics post-migration!
    }
    vtAbortIf(elm_ptr == nullptr, "Must have element locally");
    auto load = elm_ptr->getLoad(phase);
    return load;
  }

  void parseFile(std::string const& file) {
    vt_debug_print(gen, node, "parsing file {}\n", file);

    std::ifstream f(file.c_str());
    std::string line;
    bool is_load_line = false;

    int phase = 0;
    ElementIDType read_elm_id = 0;
    vt::TimeType load = 0.;

    while (std::getline(f, line)) {
      is_load_line = false;
      auto pos = line.find('[');
      if (pos != std::string::npos) {
        // this line contains subphase loads notation, i.e.,
        // phase,elm_id,phase_load,n_subphases,[subphase0_load,subphase1_load...]
        is_load_line = true;
      } else if (std::count(line.begin(), line.end(), ',') == 2) {
        // this line does not contain subphase loads notation, i.e.,
        // phase,elm_id,phase_load
        is_load_line = true;
      } // else it is comm data, i.e., phase,elm1_id,elm2_id,bytes,category

      if (is_load_line) {
        std::string nocommas = std::regex_replace(line, std::regex(","), " ");
        std::istringstream iss(nocommas);
        iss >> phase >> read_elm_id >> load;

        // only load in stats that are strictly necessary, ignoring the rest
        if (phase >= initial_phase_ && phase < initial_phase_ + phases_to_run_) {
          auto elm_id = convert_from_release_ ?
            convertReleaseStatsID(read_elm_id) : read_elm_id;
          vt_debug_print(
            gen, node,
            "reading in loads for elm={}, converted_elm={}, phase={}\n",
            read_elm_id, elm_id, phase
          );
          loads_by_obj_[elm_id][phase] = load;
          if (phase == initial_phase_)
            initial_objs_.push_back(elm_id);
        } else {
          vt_debug_print(
            gen, node,
            "skipping loads for elm={}, phase={}\n",
            read_elm_id, phase
          );
        }
      } else {
        vt_debug_print(
          gen, node,
          "skipping line: {}\n",
          line
        );
      }
    }
    f.close();
  }

  void requestObjIndices() {
    // loop over local stats objects, asking rank determined by hashing perm
    // id what the index is
    auto const this_rank = vt::theContext()->getNode();
    for (auto obj : loads_by_obj_) {
      vt::NodeType dest = findDirectoryNode(obj.first);
      if (dest != this_rank) {
        vt_debug_print(
          gen, node,
          "looking for index of object {}\n",
          obj.first
        );
        auto query = vt::makeMessage<QueryMsg>(obj.first, this_rank);
        vt::theMsg()->sendMsg<QueryMsg, RequestMapping>(dest, query);
      }
    }
  }

  void migrateObjectsHere() {
    // loop over local stats objects, asking for the corresponding collection
    // elements to be migrated here
    auto const this_rank = vt::theContext()->getNode();
    for (auto obj : initial_objs_) {
      auto idxiter = obj_id_mapping.find(obj);
      vtAssert(
        idxiter != obj_id_mapping.end(),
        "Element ID to index mapping must be known"
      );
      auto index = idxiter->second;
      vt_debug_print(
        gen, node,
        "requesting index {} (object {}) to migrate here\n",
        index, obj
      );
      proxy_[index].send<
        PlaceHolder::MigrateHereMsg, &PlaceHolder::migrateSelf
      >(this_rank);
    }
  }

  void stuffStatsIntoCollection() {
    for (auto obj : initial_objs_) {
      auto idxiter = obj_id_mapping.find(obj);
      vtAssert(
        idxiter != obj_id_mapping.end(),
        "Element ID to index mapping must be known"
      );
      auto index = idxiter->second;
      vtAssert(
        proxy_[index].tryGetLocalPtr() != nullptr,
        "should be local by now"
      );
    }
    // send a message to each object appearing in our stats files with
    // all relevant loads
    for (auto obj : loads_by_obj_) {
      auto idxiter = obj_id_mapping.find(obj.first);
      vtAssert(
        idxiter != obj_id_mapping.end(),
        "Element ID to index mapping must be known"
      );
      auto index = idxiter->second;
      vt_debug_print(
        gen, node,
        "sending stats for obj {} to index {}\n",
        obj.first, index
      );
      proxy_[index].send<
        PlaceHolder::StatsDataMsg, &PlaceHolder::recvStatsData
      >(obj.second);
    }
    loads_by_obj_.clear();
  }

private:
  std::unordered_map<
    ElementIDType,
    PlaceHolder::StatsDataMsg::ObjStatsMapType
  > loads_by_obj_;
  std::vector<ElementIDType> initial_objs_;
  int initial_phase_;
  int phases_to_run_;
  bool convert_from_release_;
  ProxyType proxy_;
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vtAbortIf(
    argc != 6,
    "Must have five arguments: <num elms per rank>, <initial phase>, <phases to run>, "
    "<stats file name>, <release?1:0>"
  );

  // number of elements per rank
  int32_t num_elms_per_rank = atoi(argv[1]);
  // initial phase to import from the stats files
  int initial_phase = atoi(argv[2]);
  // phases to run after loading object stats
  int32_t phases_to_run = atoi(argv[3]);
  // stats file name, e.g., "stats" for stats.rankid.out
  std::string stats_file = std::string{argv[4]};
  // whether or not the stats files were generated with 1.0.0
  int convert_from_release = atoi(argv[5]);

  auto const nranks = vt::theContext()->getNumNodes();
  auto const node = vt::theContext()->getNode();

  auto range = vt::Index2D(static_cast<int>(nranks), num_elms_per_rank);
  auto proxy = vt::theCollection()->constructCollective<PlaceHolder,elmIndexMap>(range);

  auto base = vt::theLBManager()->getBaseLoadModel();
  auto per_col = std::make_shared<PerCollection>(base);

  auto proxy_bits = proxy.getProxy();

  auto node_filename = fmt::format("{}.{}.out", stats_file, node);

  auto file_model = std::make_shared<FileModel>(
    base, node_filename, initial_phase, phases_to_run,
    convert_from_release, proxy
  );

  per_col->addModel(proxy_bits, file_model);

  vt::theLBManager()->setLoadModel(per_col);

  // start by moving the objects to their positions in phase initial_phase
  // and moving stats into the collection elements themselves
  if (node == 0)
    vt_print(gen, "Initializing...\n");
  vt::runInEpochCollective([=]{
    proxy.broadcastCollective<
      PlaceHolder::TestMsg, &PlaceHolder::shareMapping
    >();
  });
  vt::runInEpochCollective([=]{
    file_model->requestObjIndices();
  });
  vt::runInEpochCollective([=]{
    file_model->migrateObjectsHere();
  });
  vt::runInEpochCollective([=]{
    // find vt index of each object id in our local stats files and
    // send message with loads directly to that index
    file_model->stuffStatsIntoCollection();
  });

  if (node == 0)
    vt_print(gen, "Timestepping...\n");
  for (int i = 0; i < phases_to_run; i++) {
    if (node == 0)
      vt_print(gen, "Simulated phase {}...\n", i + initial_phase);

    // Delete this?
    proxy.broadcastCollective<PlaceHolder::TestMsg, &PlaceHolder::timestep>();

    vt::thePhase()->nextPhaseCollective();
  }

  vt::finalize();

  return 0;
}
