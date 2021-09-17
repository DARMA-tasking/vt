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
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"
#include "vt/vrt/collection/balance/read_lb.h"
#include "vt/vrt/collection/balance/lb_type.h"
#include "vt/vrt/collection/balance/node_stats.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb.h"
#include "vt/vrt/collection/balance/greedylb/greedylb.h"
#include "vt/vrt/collection/balance/rotatelb/rotatelb.h"
#include "vt/vrt/collection/balance/temperedlb/temperedlb.h"
#include "vt/vrt/collection/balance/statsmaplb/statsmaplb.h"
#include "vt/vrt/collection/balance/stats_restart_reader.h"
#include "vt/vrt/collection/balance/zoltanlb/zoltanlb.h"
#include "vt/vrt/collection/balance/randomlb/randomlb.h"
#include "vt/vrt/collection/messages/system_create.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/utils/memory/memory_usage.h"
#include "vt/vrt/collection/balance/model/load_model.h"
#include "vt/vrt/collection/balance/model/naive_persistence.h"
#include "vt/vrt/collection/balance/model/raw_data.h"
#include "vt/phase/phase_manager.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/*static*/ std::unique_ptr<LBManager> LBManager::construct() {
  auto ptr = std::make_unique<LBManager>();
  auto proxy = theObjGroup()->makeCollective<LBManager>(ptr.get());
  proxy.get()->setProxy(proxy);

  ptr->base_model_ = std::make_shared<balance::NaivePersistence>(std::make_shared<balance::RawData>());
  ptr->setLoadModel(ptr->base_model_);

  return ptr;
}

LBManager::~LBManager() = default;

LBType LBManager::decideLBToRun(PhaseType phase, bool try_file) {
  vt_debug_print(
    verbose, lb,
    "LBManager::decideLBToRun: phase={}, try_file={}, cached_phase_={}, lb={}\n",
    phase, try_file, cached_phase_, lb_names_[cached_lb_]
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
  if ((theConfig()->vt_lb_name == lb_names_[LBType::StatsMapLB]) and
      not theStatsReader()->needsLB(phase)) {
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
      for (auto&& elm : lb_names_) {
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
    phase, lb_names_[the_lb]
  );

  cached_lb_ = the_lb;
  return the_lb;
}

void LBManager::setLoadModel(std::shared_ptr<LoadModel> model) {
  model_ = model;
  auto stats = theNodeStats();
  model_->setLoads(stats->getNodeLoad(),
                   stats->getNodeSubphaseLoad(),
                   stats->getNodeComm());
}

template <typename LB>
LBManager::LBProxyType
LBManager::makeLB() {
  auto proxy = theObjGroup()->makeCollective<LB>();
  auto strat = proxy.get();
  strat->init(proxy);
  auto base_proxy = proxy.template registerBaseCollective<lb::BaseLB>();

  destroy_lb_ = [proxy]{ proxy.destroyCollective(); };

  return base_proxy;
}

void
LBManager::runLB(LBProxyType base_proxy, PhaseType phase) {
  lb::BaseLB* strat = base_proxy.get();

  runInEpochCollective("LBManager::runLB -> updateLoads", [=] {
    model_->updateLoads(phase);
  });

  runInEpochCollective("LBManager::runLB -> startLB", [=] {
    vt_debug_print(
      terse, lb,
      "LBManager: running strategy\n"
    );

    balance::CommMapType empty_comm;
    balance::CommMapType const* comm = &empty_comm;
    auto iter = theNodeStats()->getNodeComm()->find(phase);
    if (iter != theNodeStats()->getNodeComm()->end()) {
      comm = &iter->second;
    }
    strat->startLB(phase, base_proxy, model_.get(), *comm);
  });

  int32_t global_migration_count = 0;

  runInEpochCollective("LBManager::runLB -> applyMigrations", [&] {
    vt_debug_print(
      terse, lb,
      "LBManager: starting migrations\n"
    );
    strat->applyMigrations(strat->getTransfers(), [&](int32_t global_count) {
      global_migration_count = global_count;
    });
  });

  // Inform the collection manager to rebuild spanning trees if needed
  if (global_migration_count != 0) {
    theCollection()->getTypelessHolder().invokeAllGroupConstructors();
  }

  vt_debug_print(
    terse, lb,
    "LBManager: finished migrations\n"
  );
}

void LBManager::selectStartLB(PhaseType phase) {
  LBType lb = decideLBToRun(phase, true);
  startLB(phase, lb);
}

void LBManager::startLB(PhaseType phase, LBType lb) {
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
      lb_names_[lb]
    );
  }

  if (lb == LBType::NoLB) {
    // nothing to do
    return;
  }

  switch (lb) {
  case LBType::HierarchicalLB: lb_instances_["chosen"] = makeLB<lb::HierarchicalLB>(); break;
  case LBType::GreedyLB:       lb_instances_["chosen"] = makeLB<lb::GreedyLB>();       break;
  case LBType::RotateLB:       lb_instances_["chosen"] = makeLB<lb::RotateLB>();       break;
  case LBType::TemperedLB:     lb_instances_["chosen"] = makeLB<lb::TemperedLB>();     break;
  case LBType::StatsMapLB:     lb_instances_["chosen"] = makeLB<lb::StatsMapLB>();     break;
  case LBType::RandomLB:       lb_instances_["chosen"] = makeLB<lb::RandomLB>();       break;
#   if vt_check_enabled(zoltan)
  case LBType::ZoltanLB:       lb_instances_["chosen"] = makeLB<lb::ZoltanLB>();       break;
#   endif
  case LBType::NoLB:
    vtAssert(false, "LBType::NoLB is not a valid LB for collectiveImpl");
    break;
  default:
    vtAssert(false, "A valid LB must be passed to collectiveImpl");
    break;
  }

  LBProxyType base_proxy = lb_instances_["chosen"];
  runLB(base_proxy, phase);
}

/*static*/
void LBManager::printLBArgsHelp(LBType lb) {
  if (theContext()->getNode() == 0) {
    fmt::print("\nLB arguments for {}:\n\n", lb_names_[lb]);

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
        fmt::print("Argument: {}\n", arg_help.first);
        fmt::print("{}\n", arg_help.second);
      }
    } else {
      fmt::print("No LB arguments are supported by this load balancer.\n\n");
    }
  }
}

/*static*/
void LBManager::printLBArgsHelp(std::string lb_name) {
  if (lb_name.compare("NoLB") == 0) {
    for (auto&& lb : vrt::collection::balance::lb_names_) {
      vrt::collection::balance::LBManager::printLBArgsHelp(lb.first);
    }
  } else {
    for (auto&& lb : vrt::collection::balance::lb_names_) {
      if (lb_name == lb.second) {
        vrt::collection::balance::LBManager::printLBArgsHelp(lb.first);
      break;
      }
    }
  }
}

void LBManager::startup() {
  thePhase()->registerHookCollective(phase::PhaseHook::EndPostMigration, []{
    auto const phase = thePhase()->getCurrentPhase();
    theLBManager()->finishedLB(phase);
  });
}

void LBManager::finishedLB(PhaseType phase) {
  vt_debug_print(
    normal, lb,
    "finishedLB\n"
  );

  auto this_node = theContext()->getNode();

  if (this_node == 0) {
    vt_print(
      lb,
      "LBManager::finishedLB, phase={}\n", phase
    );
  }

  theNodeStats()->startIterCleanup(phase, model_->getNumPastPhasesNeeded());
  theNodeStats()->outputStatsForPhase(phase);

  // Destruct the objgroup that was used for LB
  if (destroy_lb_ != nullptr) {
    destroy_lb_();
    destroy_lb_ = nullptr;
  }
}

}}}} /* end namespace vt::vrt::collection::balance */
