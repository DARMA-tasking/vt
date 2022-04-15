/*
//@HEADER
// *****************************************************************************
//
//                                lb_manager.cc
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
#include "vt/configs/arguments/app_config.h"
#include "vt/context/context.h"
#include "vt/phase/phase_hook_enum.h"
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"
#include "vt/vrt/collection/balance/stats_msg.h"
#include "vt/vrt/collection/balance/read_lb.h"
#include "vt/vrt/collection/balance/lb_type.h"
#include "vt/vrt/collection/balance/node_lb_data.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb.h"
#include "vt/vrt/collection/balance/greedylb/greedylb.h"
#include "vt/vrt/collection/balance/rotatelb/rotatelb.h"
#include "vt/vrt/collection/balance/temperedlb/temperedlb.h"
#include "vt/vrt/collection/balance/offlinelb/offlinelb.h"
#include "vt/vrt/collection/balance/lb_data_restart_reader.h"
#include "vt/vrt/collection/balance/zoltanlb/zoltanlb.h"
#include "vt/vrt/collection/balance/randomlb/randomlb.h"
#include "vt/vrt/collection/balance/testserializationlb/testserializationlb.h"
#include "vt/vrt/collection/messages/system_create.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/utils/memory/memory_usage.h"
#include "vt/vrt/collection/balance/model/load_model.h"
#include "vt/vrt/collection/balance/model/naive_persistence.h"
#include "vt/vrt/collection/balance/model/raw_data.h"
#include "vt/vrt/collection/balance/model/proposed_reassignment.h"
#include "vt/phase/phase_manager.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/*static*/ std::unique_ptr<LBManager> LBManager::construct() {
  auto ptr = std::make_unique<LBManager>();
  auto proxy = theObjGroup()->makeCollective<LBManager>(ptr.get());
  proxy.get()->setProxy(proxy);

  ptr->base_model_ = std::make_shared<balance::NaivePersistence>(
    std::make_shared<balance::RawData>()
  );
  ptr->setLoadModel(ptr->base_model_);

  return ptr;
}

LBManager::~LBManager() = default;

LBType LBManager::decideLBToRun(PhaseType phase, bool try_file) {
  vt_debug_print(
    verbose, lb,
    "LBManager::decideLBToRun: phase={}, try_file={}, cached_phase_={}, lb={}\n",
    phase, try_file, cached_phase_, get_lb_names()[cached_lb_]
  );

  if (phase == cached_phase_) {
    return cached_lb_;
  } else {
    cached_phase_ = phase;
  }

  LBType the_lb = LBType::NoLB;

  // --vt_lb is not enabled, thus do not run the load balancer
  if (not theConfig()->vt_lb) {
    return the_lb;
  }

  //--- User-specified map without any change, thus do not run
  if ((theConfig()->vt_lb_name == get_lb_names()[LBType::OfflineLB]) and
      not theLBDataReader()->needsLB(phase)) {
    return LBType::NoLB;
  }

  auto& spec_file = theConfig()->vt_lb_file_name;
  if (spec_file != "" and try_file) {
    bool const has_spec = ReadLBSpec::openSpec(spec_file);
    if (has_spec) {
      the_lb = ReadLBSpec::getLB(phase);
    }
  } else {
    auto interval = theConfig()->vt_lb_interval;
    vtAssert(interval != 0, "LB Interval must not be 0");
    if (phase % interval == 1 || (interval == 1 && phase != 0)) {
      bool name_match = false;
      for (auto&& elm : get_lb_names()) {
        if (elm.second == theConfig()->vt_lb_name) {
          the_lb = elm.first;
          name_match = true;
          break;
        }
      }
      vtAbortIf(
        !name_match,
        fmt::format(
          "LB Name \"{}\" requested does not exist or was not enabled at "
          "compile time",
          theConfig()->vt_lb_name
        )
      );
    }
  }

  vt_debug_print(
    terse, lb,
    "LBManager::decidedLBToRun: phase={}, return lb_={}\n",
    phase, get_lb_names()[the_lb]
  );

  cached_lb_ = the_lb;
  return the_lb;
}

void LBManager::setLoadModel(std::shared_ptr<LoadModel> model) {
  model_ = model;
  auto nlb_data = theNodeLBData();
  model_->setLoads(nlb_data->getNodeLoad(),
                   nlb_data->getNodeComm());
}

template <typename LB>
LBManager::LBProxyType
LBManager::makeLB() {
  auto proxy = theObjGroup()->makeCollective<LB>();
  auto strat = proxy.get();
  strat->init(proxy);
  auto base_proxy = proxy.template castToBase<lb::BaseLB>();

  destroy_lb_ = [proxy]{ proxy.destroyCollective(); };

  return base_proxy;
}

void LBManager::defaultPostLBWork(ReassignmentMsg* msg) {
  auto reassignment = msg->reassignment;
  auto phase = msg->phase;
  auto proposed = std::make_shared<ProposedReassignment>(model_, reassignment);

  runInEpochCollective("LBManager::runLB -> computeStats", [=] {
    auto stats_cb = vt::theCB()->makeBcast<
      LBManager, StatsMsgType, &LBManager::statsHandler
    >(proxy_);
    before_lb_stats_ = false;
    computeStatistics(proposed, false, phase, stats_cb);
  });

  // Inform the collection manager to rebuild spanning trees if needed
  if (reassignment->global_migration_count != 0) {
    theCollection()->getTypelessHolder().invokeAllGroupConstructors();
  }

  last_phase_info_->migration_count = reassignment->global_migration_count;
  last_phase_info_->ran_lb = true;

  applyReassignment(reassignment);

  // Inform the collection manager to rebuild spanning trees if needed
  if (reassignment->global_migration_count != 0) {
    theCollection()->getTypelessHolder().invokeAllGroupConstructors();
  }

  vt_debug_print(
    terse, lb,
    "LBManager: finished migrations\n"
  );
}

void
LBManager::runLB(
  LBProxyType base_proxy, PhaseType phase, vt::Callback<ReassignmentMsg> cb
) {
  runInEpochCollective("LBManager::runLB -> updateLoads", [=] {
    model_->updateLoads(phase);
  });

  runInEpochCollective("LBManager::runLB -> computeStats", [=] {
    auto stats_cb = vt::theCB()->makeBcast<
      LBManager, StatsMsgType, &LBManager::statsHandler
    >(proxy_);
    before_lb_stats_ = true;
    computeStatistics(model_, false, phase, stats_cb);
  });

  elm::CommMapType empty_comm;
  elm::CommMapType const* comm = &empty_comm;
  auto iter = theNodeLBData()->getNodeComm()->find(phase);
  if (iter != theNodeLBData()->getNodeComm()->end()) {
    comm = &iter->second;
  }

  vt_debug_print(terse, lb, "LBManager: running strategy\n");

  lb::BaseLB* strat = base_proxy.get();
  auto reassignment = strat->startLB(
    phase, base_proxy, model_.get(), stats, *comm, total_load
  );
  cb.send(reassignment, phase);
}

void LBManager::selectStartLB(PhaseType phase) {
  namespace ph = std::placeholders;
  auto post_lb_ptr = std::mem_fn(&LBManager::defaultPostLBWork);
  auto post_lb_fn = std::bind(post_lb_ptr, this, ph::_1);
  auto cb = theCB()->makeFunc<ReassignmentMsg>(
    vt::pipe::LifetimeEnum::Once, post_lb_fn
  );
  selectStartLB(phase, cb);
}

void LBManager::selectStartLB(
  PhaseType phase, vt::Callback<ReassignmentMsg> cb
) {
  LBType lb = decideLBToRun(phase, true);
  startLB(phase, lb, cb);
}

void LBManager::startLB(
  PhaseType phase, LBType lb, vt::Callback<ReassignmentMsg> cb
) {
  vt_debug_print(
    normal, lb,
    "LBManager::startLB: phase={}\n", phase
  );

  auto const& this_node = theContext()->getNode();

  if (this_node == 0 and not theConfig()->vt_lb_quiet) {
    vt_debug_print(
      terse, lb,
      "LBManager::startLB: phase={}, balancer={}, name={}\n",
      phase,
      static_cast<typename std::underlying_type<LBType>::type>(lb),
      get_lb_names()[lb]
    );
  }

  if (lb == LBType::NoLB) {
    last_phase_info_->migration_count = 0;
    last_phase_info_->ran_lb = false;

    runInEpochCollective("LBManager::noLB -> updateLoads", [=] {
      model_->updateLoads(phase);
    });

    runInEpochCollective("LBManager::noLB -> computeStats", [=] {
      before_lb_stats_ = true;
      auto stats_cb = vt::theCB()->makeBcast<
        LBManager, StatsMsgType, &LBManager::statsHandler
      >(proxy_);
      before_lb_stats_ = true;
      computeStatistics(model_, false, phase, stats_cb);
    });
    // nothing to do
    return;
  }

  switch (lb) {
  case LBType::HierarchicalLB:      lb_instances_["chosen"] = makeLB<lb::HierarchicalLB>();      break;
  case LBType::GreedyLB:            lb_instances_["chosen"] = makeLB<lb::GreedyLB>();            break;
  case LBType::RotateLB:            lb_instances_["chosen"] = makeLB<lb::RotateLB>();            break;
  case LBType::TemperedLB:          lb_instances_["chosen"] = makeLB<lb::TemperedLB>();          break;
  case LBType::OfflineLB:           lb_instances_["chosen"] = makeLB<lb::OfflineLB>();           break;
  case LBType::RandomLB:            lb_instances_["chosen"] = makeLB<lb::RandomLB>();            break;
#   if vt_check_enabled(zoltan)
  case LBType::ZoltanLB:            lb_instances_["chosen"] = makeLB<lb::ZoltanLB>();            break;
#   endif
  case LBType::TestSerializationLB: lb_instances_["chosen"] = makeLB<lb::TestSerializationLB>(); break;
  case LBType::NoLB:
    vtAssert(false, "LBType::NoLB is not a valid LB for collectiveImpl");
    break;
  default:
    vtAssert(false, "A valid LB must be passed to collectiveImpl");
    break;
  }

  LBProxyType base_proxy = lb_instances_["chosen"];
  runLB(base_proxy, phase, cb);
}

/*static*/
void LBManager::printLBArgsHelp(LBType lb) {
  auto sep = fmt::format("{}{:-^120}{}\n", debug::bd_green(), "", debug::reset());
  fmt::print(sep);
  fmt::print(
    "{}{}LB arguments for {}{}{}:\n",
    debug::vtPre(), debug::green(), debug::magenta(),
    get_lb_names()[lb], debug::reset()
  );
  fmt::print(sep);
  fmt::print("\n");

  std::unordered_map<std::string, std::string> help;

  switch (lb) {
  case LBType::HierarchicalLB:
    help = lb::HierarchicalLB::getInputKeysWithHelp();
    break;
  case LBType::GreedyLB:
    help = lb::GreedyLB::getInputKeysWithHelp();
    break;
  case LBType::RotateLB:
    help = lb::RotateLB::getInputKeysWithHelp();
    break;
  case LBType::TemperedLB:
    help = lb::TemperedLB::getInputKeysWithHelp();
    break;
  case LBType::RandomLB:
    help = lb::RandomLB::getInputKeysWithHelp();
    break;
  case LBType::OfflineLB:
    help = lb::OfflineLB::getInputKeysWithHelp();
    break;
# if vt_check_enabled(zoltan)
  case LBType::ZoltanLB:
    help = lb::ZoltanLB::getInputKeysWithHelp();
    break;
# endif
  case LBType::TestSerializationLB:
    help = lb::TestSerializationLB::getInputKeysWithHelp();
    break;
  case LBType::NoLB:
    // deliberately skip retrieving arguments
    break;
  default:
    fmt::print("Documentation has not been provided for this LB.\n\n");
    return;
    break;
  }

  if (help.size() > 0) {
    for (auto &arg_help : help) {
      fmt::print(
        "{}Argument: {}{}{}",
        debug::yellow(), debug::red(), arg_help.first, debug::reset()
      );
      fmt::print("{}{}{}\n", debug::reset(), arg_help.second, debug::reset());
    }
  } else {
    fmt::print("No LB arguments are supported by this load balancer.\n\n");
  }
}

/*static*/
void LBManager::printLBArgsHelp(std::string lb_name) {
  if (lb_name.compare("NoLB") == 0) {
    for (auto&& lb : vrt::collection::balance::get_lb_names()) {
      vrt::collection::balance::LBManager::printLBArgsHelp(lb.first);
    }
  } else {
    for (auto&& lb : vrt::collection::balance::get_lb_names()) {
      if (lb_name == lb.second) {
        vrt::collection::balance::LBManager::printLBArgsHelp(lb.first);
        break;
      }
    }
  }
}

void LBManager::startup() {
  last_phase_info_ = std::make_unique<lb::PhaseInfo>();

  thePhase()->registerHookRooted(phase::PhaseHook::Start, []{
    thePhase()->setStartTime();
  });

  thePhase()->registerHookCollective(phase::PhaseHook::EndPostMigration, [this]{
    auto const phase = thePhase()->getCurrentPhase();
    thePhase()->printSummary(last_phase_info_.get());
    theLBManager()->finishedLB(phase);
  });
}

void LBManager::destroyLB() {
  // Destruct the objgroup that was used for LB
  if (destroy_lb_ != nullptr) {
    destroy_lb_();
    destroy_lb_ = nullptr;
  }
}

void LBManager::finishedLB(PhaseType phase) {
  vt_debug_print(
    normal, lb,
    "finishedLB\n"
  );

  theNodeLBData()->startIterCleanup(phase, model_->getNumPastPhasesNeeded());
  theNodeLBData()->outputLBDataForPhase(phase);

  destroyLB();
}

void LBManager::statsHandler(StatsMsgType* msg) {
  auto in_stat_vec = msg->getConstVal();

  //vt_print(lb, "before_lb_stats_={}\n", before_lb_stats_);

  for (auto&& st : in_stat_vec) {
    auto stat     = st.stat_;
    auto max      = st.max();
    auto min      = st.min();
    auto avg      = st.avg();
    auto sum      = st.sum();
    auto npr      = st.npr();
    auto car      = st.N_;
    auto imb      = st.I();
    auto var      = st.var();
    auto stdv     = st.stdv();
    auto skew     = st.skew();
    auto krte     = st.krte();

    stats[stat][lb::StatisticQuantity::max] = max;
    stats[stat][lb::StatisticQuantity::min] = min;
    stats[stat][lb::StatisticQuantity::avg] = avg;
    stats[stat][lb::StatisticQuantity::sum] = sum;
    stats[stat][lb::StatisticQuantity::npr] = npr;
    stats[stat][lb::StatisticQuantity::car] = car;
    stats[stat][lb::StatisticQuantity::var] = var;
    stats[stat][lb::StatisticQuantity::npr] = npr;
    stats[stat][lb::StatisticQuantity::imb] = imb;
    stats[stat][lb::StatisticQuantity::std] = stdv;
    stats[stat][lb::StatisticQuantity::skw] = skew;
    stats[stat][lb::StatisticQuantity::kur] = krte;

    if (stat == lb::Statistic::Rank_load_model) {
      if (before_lb_stats_) {
        last_phase_info_->max_load = max;
        last_phase_info_->avg_load = avg;
        last_phase_info_->imb_load = imb;
      } else {
        last_phase_info_->max_load_post_lb = max;
        last_phase_info_->avg_load_post_lb = avg;
        last_phase_info_->imb_load_post_lb = imb;
      }
    } else if (stat == lb::Statistic::Object_load_model) {
      if (before_lb_stats_) {
        last_phase_info_->max_obj = max;
      } else {
        last_phase_info_->max_obj_post_lb = max;
      }
    }

    // if (theContext()->getNode() == 0) {
    //   vt_print(
    //     lb,
    //     "LBManager: Statistic={}: "
    //     " max={:.2f}, min={:.2f}, sum={:.2f}, avg={:.2f}, var={:.2f},"
    //     " stdev={:.2f}, nproc={}, cardinality={} skewness={:.2f}, kurtosis={:.2f},"
    //     " npr={}, imb={:.2f}, num_stats={}\n",
    //     lb::get_lb_stat_name()[stat],
    //     max, min, sum, avg, var, stdv, npr, car, skew, krte, npr, imb,
    //     stats.size()
    //   );
    // }
  }
}

balance::LoadData reduceVec(
  lb::Statistic stat, std::vector<balance::LoadData>&& vec
) {
  balance::LoadData reduce_ld(stat, 0.0f);
  if (vec.size() == 0) {
    return reduce_ld;
  } else {
    for (std::size_t i = 1; i < vec.size(); i++) {
      vec[0] = vec[0] + vec[i];
    }
    return vec[0];
  }
}

void LBManager::computeStatistics(
  std::shared_ptr<LoadModel> model, bool comm_collectives, PhaseType phase,
  vt::Callback<StatsMsgType> cb
) {
  vt_debug_print(
    normal, lb,
    "computeStatistics\n"
  );

  using ReduceOp = collective::PlusOp<std::vector<balance::LoadData>>;

  total_load = 0.;
  std::vector<balance::LoadData> obj_load_model;
  for (auto elm : *model) {
    auto work = model->getWork(
      elm, {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE}
    );
    obj_load_model.emplace_back(
      LoadData{lb::Statistic::Object_load_model, work}
    );
    total_load += work;
  }

  elm::CommMapType empty_comm;
  elm::CommMapType const* comm_data = &empty_comm;
  auto iter = theNodeLBData()->getNodeComm()->find(phase);
  if (iter != theNodeLBData()->getNodeComm()->end()) {
    comm_data = &iter->second;
  }

  std::vector<LoadData> lstats;
  lstats.emplace_back(LoadData{lb::Statistic::Rank_load_model, total_load});
  lstats.emplace_back(reduceVec(
    lb::Statistic::Object_load_model, std::move(obj_load_model)
  ));

  double comm_load = 0.0;
  for (auto&& elm : *comm_data) {
    if (not comm_collectives and isCollectiveComm(elm.first.cat_)) {
      continue;
    }
    if (elm.first.onNode() or elm.first.selfEdge()) {
      continue;
    }
    //vt_print(lb, "comm_load={}, elm={}\n", comm_load, elm.second.bytes);
    comm_load += elm.second.bytes;
  }

  lstats.emplace_back(LoadData{lb::Statistic::Rank_comm, comm_load});

  std::vector<balance::LoadData> obj_comm;
  for (auto&& elm : *comm_data) {
    // Only count object-to-object direct edges in the Object_comm statistics
    if (elm.first.cat_ == elm::CommCategory::SendRecv and not elm.first.selfEdge()) {
      obj_comm.emplace_back(
        LoadData{lb::Statistic::Object_comm, elm.second.bytes}
      );
    }
  }

  lstats.emplace_back(reduceVec(
    lb::Statistic::Object_comm, std::move(obj_comm)
  ));

  auto msg = makeMessage<StatsMsgType>(std::move(lstats));
  proxy_.template reduce<ReduceOp>(msg,cb);
}

bool LBManager::isCollectiveComm(elm::CommCategory cat) const {
  bool is_collective =
    cat == elm::CommCategory::Broadcast or
    cat == elm::CommCategory::CollectionToNodeBcast or
    cat == elm::CommCategory::NodeToCollectionBcast;
  return is_collective;
}

}}}} /* end namespace vt::vrt::collection::balance */
