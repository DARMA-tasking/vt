/*
//@HEADER
// *****************************************************************************
//
//                               phase_manager.cc
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

#include "vt/phase/phase_manager.h"
#include "vt/objgroup/headers.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/timing/timing.h"

namespace vt { namespace phase {

PhaseManager::PhaseManager() {
  setStartTime();
}

/*static*/ std::unique_ptr<PhaseManager> PhaseManager::construct() {
  auto ptr = std::make_unique<PhaseManager>();
  auto proxy = theObjGroup()->makeCollective<PhaseManager>(ptr.get());
  proxy.get()->proxy_ = proxy.getProxy();;
  return ptr;
}

PhaseHookID
PhaseManager::registerHookCollective(PhaseHook type, ActionType trigger) {
  vtAssertNot(
    in_next_phase_collective_, "Must not be in next phase to register"
  );

  bool const is_collective = true;
  auto const type_bits = static_cast<HookIDType>(type);
  auto const hook_id = next_collective_hook_id_++;
  collective_hooks_[type_bits][hook_id] = trigger;

  vt_debug_print(
    verbose, phase,
    "PhaseManager::registerHookCollective: type={}, hook_id={}\n",
    type_bits, hook_id
  );

  return PhaseHookID{type, hook_id, is_collective};
}

PhaseHookID
PhaseManager::registerHookRooted(PhaseHook type, ActionType trigger) {
  vtAssertNot(
    in_next_phase_collective_, "Must not be in next phase to register"
  );

  bool const is_collective = false;
  auto const type_bits = static_cast<HookIDType>(type);
  auto const hook_id = next_rooted_hook_id_++;
  rooted_hooks_[type_bits][hook_id] = trigger;

  vt_debug_print(
    verbose, phase,
    "PhaseManager::registerHookRooted: type={}, hook_id={}\n",
    type_bits, hook_id
  );

  return PhaseHookID{type, hook_id, is_collective};
}

void PhaseManager::unregisterHook(PhaseHookID hook) {
  vtAssertNot(
    in_next_phase_collective_, "Must not be in next phase to unregister"
  );

  auto const type = static_cast<HookIDType>(hook.getType());
  auto const id = hook.getID();
  auto const is_collective = hook.getIsCollective();

  vt_debug_print(
    verbose, phase,
    "PhaseManager::unregisterHook: type={}, id={}, is_collective={}\n",
    type, id, is_collective
  );

  auto& hooks = is_collective ? collective_hooks_ : rooted_hooks_;
  auto iter = hooks[type].find(id);
  if (iter != hooks[type].end()) {
    hooks[type].erase(iter);
  } else {
    vtAbort("Could not find registered hook ID to erase");
  }
}

void PhaseManager::startup() {
  // This is the last chance to fire any starting hooks for the very first phase
  runHooks(PhaseHook::Start);
}

struct NextMsg : collective::ReduceNoneMsg {};

void PhaseManager::nextPhaseCollective() {
  vtAbortIf(
    in_next_phase_collective_,
    "A call to nextPhaseCollective has already been invoked."
    " It must return before it is invoked again"
  );
  in_next_phase_collective_ = true;

  vt_debug_print(
    normal, phase,
    "PhaseManager::nextPhaseCollective: cur_phase_={}\n", cur_phase_
  );

  // Convert bits to typed proxy
  auto proxy = objgroup::proxy::Proxy<PhaseManager>(proxy_);

  // Start with a reduction to sure all nodes are ready for this
  auto cb = theCB()->makeBcast<PhaseManager, NextMsg, &PhaseManager::nextPhaseReduce>(proxy);
  auto msg = makeMessage<NextMsg>();
  proxy.reduce(msg.get(), cb);

  theSched()->runSchedulerWhile([this]{ return not reduce_next_phase_done_; });
  reduce_next_phase_done_ = false;

  vt_debug_print(
    normal, phase,
    "PhaseManager::nextPhaseCollective: cur_phase_={}, reduce done, running "
    "hooks\n", cur_phase_
  );

  runHooks(PhaseHook::End);
  runHooks(PhaseHook::EndPostMigration);

  cur_phase_++;

  vt_debug_print(
    normal, phase,
    "PhaseManager::nextPhaseCollective: starting next phase: cur_phase_={}\n",
    cur_phase_
  );

  runHooks(PhaseHook::Start);

  // Start with a reduction to sure all nodes are ready for this
  auto cb2 = theCB()->makeBcast<PhaseManager, NextMsg, &PhaseManager::nextPhaseDone>(proxy);
  auto msg2 = makeMessage<NextMsg>();
  proxy.reduce(msg2.get(), cb2);

  theSched()->runSchedulerWhile([this]{ return not reduce_finished_; });
  reduce_finished_ = false;

  in_next_phase_collective_ = false;
}

void PhaseManager::nextPhaseReduce(NextMsg* msg) {
  reduce_next_phase_done_ = true;
}

void PhaseManager::nextPhaseDone(NextMsg* msg) {
  reduce_finished_ = true;
}

void PhaseManager::runHooks(PhaseHook type) {
  auto const type_bits = static_cast<HookIDType>(type);

  // start out running all rooted hooks of a particular type
  {
    auto iter = rooted_hooks_.find(type_bits);
    if (iter != rooted_hooks_.end()) {
      for (auto&& fn : iter->second) {
        runInEpochRooted("PhaseManager::runHooks", [=]{ fn.second(); });
      }
    }
  }

  // then, run collective hooks that should be symmetric across nodes
  {
    auto iter = collective_hooks_.find(type_bits);
    if (iter != collective_hooks_.end()) {
      // note, this second is a map, so they are ordered across nodes
      for (auto&& fn : iter->second) {
        runInEpochCollective("PhaseManager::runHooks", [=]{ fn.second(); });
      }
    }
  }
}

void PhaseManager::runHooksManual(PhaseHook type) {
  runHooks(type);
}

void PhaseManager::setStartTime() {
  start_time_ = timing::getCurrentTime();
}

void PhaseManager::printSummary(vrt::collection::lb::PhaseInfo* last_phase_info) {
  if (theContext()->getNode() == 0) {
    TimeTypeWrapper const total_time = timing::getCurrentTime() - start_time_;
    vt_print(
      phase,
      "phase={}, duration={}, rank_max_compute_time={}, rank_avg_compute_time={}, imbalance={:.3f}, "
      "grain_max_time={}, migration count={}\n",
      cur_phase_,
      total_time,
      TimeTypeWrapper(last_phase_info->max_load),
      TimeTypeWrapper(last_phase_info->avg_load),
      last_phase_info->imb_load,
      TimeTypeWrapper(last_phase_info->max_obj),
      last_phase_info->migration_count
    );
    // vt_print(
    //   phase,
    //   "POST phase={}, total time={}, max_load={}, avg_load={}, imbalance={:.3f}, max_obj={}, migration count={}\n",
    //   cur_phase_,
    //   total_time,
    //   TimeTypeWrapper(last_phase_info->max_load_post_lb),
    //   TimeTypeWrapper(last_phase_info->avg_load_post_lb),
    //   last_phase_info->imb_load_post_lb,
    //   TimeTypeWrapper(last_phase_info->max_obj_post_lb),
    //   last_phase_info->migration_count
    // );

    auto compute_speedup = [](double t1, double t2) -> double {
       return t1 / t2;
    };
    auto compute_percent_improvement = [&](double t1, double t2) -> double {
      return (t1 - t2) / t1 * 100.0;
    };

    if (not last_phase_info->ran_lb) {
      auto speedup = compute_speedup(
        last_phase_info->max_load, last_phase_info->avg_load
      );
      auto percent_improvement = compute_percent_improvement(
        last_phase_info->max_load, last_phase_info->avg_load
      );
      // @todo: where should we cut off this print?
      // if (percent_improvement > 3.0) {
        vt_print(
          phase,
          "Ideal load balance would run {:.2f}x faster "
          "(or take {:.2f}% less time)\n",
          speedup, percent_improvement
        );
      // }
    } else if (last_phase_info->ran_lb and last_phase_info->migration_count > 0)  {
      auto speedup = compute_speedup(
        last_phase_info->max_load_post_lb, last_phase_info->avg_load_post_lb
      );
      auto percent_improvement = compute_percent_improvement(
        last_phase_info->max_load_post_lb, last_phase_info->avg_load_post_lb
      );
      vt_print(
        phase,
        "After load balancing, expected execution should get a {:.2f}x speedup"
        " (or take {:.2f}% less time)\n",
        speedup, percent_improvement
      );
    }
    if (last_phase_info->max_obj > last_phase_info->avg_load) {
      auto speedup = compute_speedup(
        last_phase_info->max_load, last_phase_info->max_obj
      );
      auto percent_improvement = compute_percent_improvement(
        last_phase_info->max_load, last_phase_info->max_obj
      );
      vt_print(
        phase,
        "Largest grain object limits load balancing improvement to a "
        "{:.2f}x speedup (or take {:.2f}% less time)\n",
        speedup, percent_improvement
      );
    }
    fflush(stdout);
  }
}

}} /* end namespace vt::phase */
