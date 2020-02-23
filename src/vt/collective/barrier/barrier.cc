/*
//@HEADER
// *****************************************************************************
//
//                                  barrier.cc
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

#include "vt/collective/barrier/barrier.h"
#include "vt/collective/collective_alg.h"
#include "vt/messaging/active.h"

namespace vt { namespace collective { namespace barrier {

Barrier::Barrier() :
  tree::Tree(tree::tree_cons_tag_t)
{ }

/*static*/ void Barrier::barrierUp(BarrierMsg* msg) {
  theCollective()->barrierUp(
    msg->is_named, msg->is_wait, msg->barrier, msg->skip_term
  );
}

/*static*/ void Barrier::barrierDown(BarrierMsg* msg) {
  theCollective()->barrierDown(msg->is_named, msg->is_wait, msg->barrier);
}

Barrier::BarrierStateType& Barrier::insertFindBarrier(
  bool const& is_named, bool const& is_wait, BarrierType const& barrier,
  ActionType cont_action
) {
  auto& state = is_named ? named_barrier_state_ : unnamed_barrier_state_;

  auto iter = state.find(barrier);
  if (iter == state.end()) {
    state.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(barrier),
      std::forward_as_tuple(BarrierStateType{is_named, barrier, is_wait})
    );
    iter = state.find(barrier);
  }

  if (cont_action != nullptr) {
    vtAssert(
      (not is_wait),
      "Barrier must not be waiting kind if action is associated"
    );

    iter->second.cont_action = cont_action;
  }

  return iter->second;
}

void Barrier::removeBarrier(
  bool const& is_named, bool const& is_wait, BarrierType const& barrier
) {
  auto& state = is_named ? named_barrier_state_ : unnamed_barrier_state_;

  auto iter = state.find(barrier);
  vtAssert(
    iter != state.end(), "Barrier must exist at this point"
  );

  state.erase(iter);
}

BarrierType Barrier::newNamedCollectiveBarrier() {
  return cur_named_coll_barrier_++;
}

BarrierType Barrier::newNamedBarrier() {
  BarrierType const next_barrier = cur_named_barrier_++;
  NodeType const cur_node = theContext()->getNode();
  BarrierType const cur_node_shift = static_cast<BarrierType>(cur_node) << 32;
  BarrierType const barrier_name = next_barrier | cur_node_shift;
  return barrier_name;
}

void Barrier::waitBarrier(
  ActionType poll_action, BarrierType const& named, bool const skip_term
) {
  bool const is_wait = true;
  bool const is_named = named != no_barrier;

  BarrierType const barrier = is_named ? named : cur_unnamed_barrier_++;

  auto& barrier_state = insertFindBarrier(is_named, is_wait, barrier);

  debug_print(
    barrier, node,
    "waitBarrier: named={}, barrier={}\n", is_named, barrier
  );

  barrierUp(is_named, is_wait, barrier, skip_term);

  if (not barrier_state.released) {
    auto sched = theSched()->beginNestedScheduling();
    while (not barrier_state.released) {
      sched.runScheduler();
      if (poll_action) {
        poll_action();
      }
    }
  }

  debug_print(
    barrier, node,
    "waitBarrier: released: named={}, barrier={}\n", is_named, barrier
  );

  removeBarrier(is_named, is_wait, barrier);
}

void Barrier::contBarrier(
  ActionType fn, BarrierType const& named, bool const skip_term
) {
  debug_print(
    barrier, node,
    "contBarrier: named_barrier={}, skip_term={}\n", named, skip_term
  );

  bool const is_wait = false;
  bool const is_named = named != no_barrier;

  BarrierType const barrier = is_named ? named : cur_unnamed_barrier_++;

  insertFindBarrier(is_named, is_wait, barrier, fn);

  debug_print(
    barrier, node,
    "contBarrier: named={}, barrier={}\n", is_named, barrier
  );

  barrierUp(is_named, is_wait, barrier, skip_term);
}

void Barrier::barrierDown(
  bool const& is_named, bool const& is_wait, BarrierType const& barrier
) {
  auto& barrier_state = insertFindBarrier(is_named, is_wait, barrier);

  debug_print(
    barrier, node,
    "barrierUp: invoking: named={}, wait={}, barrier={}\n",
    is_named, is_wait, barrier
  );

  barrier_state.released = true;

  if (not is_wait and barrier_state.cont_action != nullptr) {
    barrier_state.cont_action();
  }
}

void Barrier::barrierUp(
  bool const& is_named, bool const& is_wait, BarrierType const& barrier,
  bool const& skip_term
) {
  auto const& num_children = getNumChildren();
  bool const& is_root = isRoot();
  auto const& parent = getParent();

  // ToDo: Why we call this function again? (if you come from contBarrier,
  // ToDo: this is already called)
  auto& barrier_state = insertFindBarrier(is_named, is_wait, barrier);

  barrier_state.recv_event_count += 1;

  bool const is_ready = barrier_state.recv_event_count == num_children + 1;

  debug_print(
    barrier, node,
    "barrierUp: invoking: named={}, wait={}, ready={}, events={}, barrier={}\n",
    is_named, is_wait, is_ready, barrier_state.recv_event_count, barrier
  );

  if (is_ready) {
    if (not is_root) {
      auto msg = makeSharedMessage<BarrierMsg>(is_named, barrier, is_wait);
      // system-level barriers can choose to skip the termination protocol
      if (skip_term) {
        theMsg()->markAsTermMessage(msg);
      }
      debug_print(
        barrier, node,
        "barrierUp: barrier={}\n", barrier
      );
      theMsg()->sendMsg<BarrierMsg, barrierUp>(parent, msg);
    } else {
      auto msg = makeSharedMessage<BarrierMsg>(is_named, barrier, is_wait);
      // system-level barriers can choose to skip the termination protocol
      if (skip_term) {
        theMsg()->markAsTermMessage(msg);
      }
      debug_print(
        barrier, node,
        "barrierDown: barrier={}\n", barrier
      );
      theMsg()->broadcastMsg<BarrierMsg, barrierDown>(msg);
      barrierDown(is_named, is_wait, barrier);
    }
  }
}

}}}  // end namespace vt::collective::barrier
