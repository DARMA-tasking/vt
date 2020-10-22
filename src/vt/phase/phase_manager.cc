/*
//@HEADER
// *****************************************************************************
//
//                               phase_manager.cc
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

#include "vt/phase/phase_manager.h"
#include "vt/objgroup/headers.h"
#include "vt/pipe/pipe_manager.h"

namespace vt { namespace phase {

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
  return PhaseHookID{type, hook_id, is_collective};
}

PhaseHookID
PhaseManager::registerHookRooted(PhaseHook type, ActionType trigger) {
  vtAssertNot(
    in_next_phase_collective_, "Must not be in next phase to register"
  );

  bool const is_collective = true;
  auto const type_bits = static_cast<HookIDType>(type);
  auto const hook_id = next_rooted_hook_id_++;
  rooted_hooks_[type_bits][hook_id] = trigger;
  return PhaseHookID{type, hook_id, is_collective};
}

void PhaseManager::unregisterHook(PhaseHookID hook) {
  vtAssertNot(
    in_next_phase_collective_, "Must not be in next phase to unregister"
  );

  auto const type = static_cast<HookIDType>(hook.getType());
  auto const id = hook.getID();
  auto const is_collective = hook.getIsCollective();
  auto& hooks = is_collective ? collective_hooks_ : rooted_hooks_;
  auto iter = hooks[type].find(id);
  if (iter != hooks[type].end()) {
    hooks[type].erase(iter);
  } else {
    vtAssert(false, "Could not find registered hook ID to erase");
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

  // Convert bits to typed proxy
  auto proxy = objgroup::proxy::Proxy<PhaseManager>(proxy_);

  // Start with a reduction to sure all nodes are ready for this
  auto cb = theCB()->makeBcast<PhaseManager, NextMsg, &PhaseManager::nextPhaseReduce>(proxy);
  auto msg = makeMessage<NextMsg>();
  proxy.reduce(msg.get(), cb);

  theSched()->runSchedulerWhile([this]{ return not reduce_next_phase_done_; });
  reduce_next_phase_done_ = false;

  runHooks(PhaseHook::End);
  runHooks(PhaseHook::EndPostMigration);

  cur_phase_++;
  runHooks(PhaseHook::Start);

  in_next_phase_collective_ = false;
}

void PhaseManager::nextPhaseReduce(NextMsg* msg) {
  reduce_next_phase_done_ = true;
}

void PhaseManager::runHooks(PhaseHook type) {
  auto const type_bits = static_cast<HookIDType>(type);

  // start out running all rooted hooks of a particular type
  {
    auto iter = rooted_hooks_.find(type_bits);
    if (iter != rooted_hooks_.end()) {
      if (iter->second.size() > 0) {
        for (auto&& fn : iter->second) {
          runInEpochRooted([=]{ fn.second(); });
        }
      }
    }
  }

  // then, run collective hooks that should be symmetric across nodes
  {
    auto iter = collective_hooks_.find(type_bits);
    if (iter != collective_hooks_.end()) {
      if (iter->second.size() > 0) {
        // note, this second is a map, so they are ordered across nodes
        for (auto&& fn : iter->second) {
          runInEpochCollective([=]{ fn.second(); });
        }
      }
    }
  }
}

}} /* end namespace vt::phase */
