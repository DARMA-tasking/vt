/*
//@HEADER
// *****************************************************************************
//
//                                  manager.cc
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

#include "vt/config.h"
#include "vt/runtime/runtime.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/base/base.h"
#include "vt/vrt/collection/manager.h"
#include "vt/vrt/collection/balance/lb_invoke/invoke.h"

namespace vt { namespace vrt { namespace collection {

CollectionManager::CollectionManager() {
  balance::LBManager::init();

  if (ArgType::vt_print_buffered_msgs) {
    // Stamp in the NodeType bits for this_node in the high bits to create a
    // globally distinct buffer identifier
    auto node_stamp = static_cast<decltype(buffer_id_)>(theContext()->getNode());
    node_stamp <<= (sizeof(decltype(buffer_id_)) - sizeof(NodeType)) * 8;
    buffer_id_ |= node_stamp;
  }
}

/*virtual*/ CollectionManager::~CollectionManager() {
  cleanupAll<>();

  // Statistics output when LB is enabled and appropriate flag is enabled
#if backend_check_enabled(lblite)
  if (ArgType::vt_lb_stats) {
    balance::ProcStats::outputStatsFile();
    balance::ProcStats::clearStats();
  }
#endif

  // Destroy the LBManager
  balance::LBManager::destroy();
}

#if backend_check_enabled(lblite)
struct StartRootedMsg : vt::Message {
  StartRootedMsg() = default;
  explicit StartRootedMsg(PhaseType in_phase) : phase_(in_phase) { }
  PhaseType phase_;
};

static void startRootedBroadcast(StartRootedMsg* msg) {
  theCollection()->startPhaseCollective(nullptr, msg->phase_);
}
#endif

void CollectionManager::startPhaseRooted(
  ActionFinishedLBType fn, PhaseType lb_phase
) {
#if backend_check_enabled(lblite)
  auto msg = makeSharedMessage<StartRootedMsg>(lb_phase);
  theMsg()->broadcastMsg<StartRootedMsg, startRootedBroadcast>(msg);
  startPhaseCollective(fn, lb_phase);
#else
  if (fn != nullptr) {
    fn();
  }
#endif
}

void CollectionManager::startPhaseCollective(
  ActionFinishedLBType fn, PhaseType lb_phase
) {
#if backend_check_enabled(lblite)
  UniversalIndexHolder<>::runLB(lb_phase);
  if (fn != nullptr) {
    theTerm()->produce(term::any_epoch_sentinel);
    lb_continuations_.push_back(fn);
  } else {
    auto proxy = balance::LBManager::getProxy();
    proxy.get()->waitLBCollective();
  }
#else
  if (fn != nullptr) {
    fn();
  }
#endif
#if backend_check_enabled(trace_enabled)
  theTrace()->flushTracesFile(false);
#endif
}

DispatchBasePtrType
getDispatcher(auto_registry::AutoHandlerType const& han) {
  return theCollection()->getDispatcher(han);
}

void releaseLBPhase(CollectionPhaseMsg* msg) {
  CollectionManager::releaseLBPhase<>(msg);
}

balance::ElementIDType CollectionManager::getCurrentContextPerm() const {
  return cur_context_perm_elm_id_;
}

balance::ElementIDType CollectionManager::getCurrentContextTemp() const {
  return cur_context_temp_elm_id_;
}

void CollectionManager::setCurrentContext(
  balance::ElementIDType perm, balance::ElementIDType temp
) {
  cur_context_perm_elm_id_ = perm;
  cur_context_temp_elm_id_ = temp;
}

void CollectionManager::schedule(ActionType action) {
  theSched()->enqueue(action);
}

bool CollectionManager::progress() {
  return false;
}

}}} /* end namespace vt::vrt::collection */
