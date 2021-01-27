/*
//@HEADER
// *****************************************************************************
//
//                               epoch_window.cc
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

#include "vt/epoch/epoch_window.h"
#include "vt/epoch/epoch_manip.h"
#include "vt/collective/reduce/reduce_scope.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/termination/interval/integral_set_intersect.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/epoch/epoch_garbage_collect.h"
#include "vt/epoch/garbage_collect_msg.h"

#include <fmt/ostream.h>

namespace vt { namespace epoch {

EpochWindow::EpochWindow(EpochType epoch) {
  // Make a copy for manipulation
  EpochType arch_epoch = epoch;

  // Set the sequence to zero to create the archetype that can be compared
  // easily to incoming epochs to check that they match all the control bit
  // fields. For efficiency, the window relies on the sequentiality of
  // same-typed epochs to create a semi-contiguous window of terminated epochs.
  EpochManip::setSeq(arch_epoch,0);
  archetype_epoch_ = arch_epoch;

  // Set all non-control bits (sequence bits to max value) to build the max
  // epoch allowed for this archetype
  EpochType max_epoch = archetype_epoch_;
  EpochManip::setSeq(max_epoch, (~0ull-1));

  // The minimum epoch within this archetype always starts with the sequence
  // number at 1; this saves space for the global, collective epoch which is
  // zero. In fact, for simplicity, the global epoch is all zeros given the
  // control bit scheme
  EpochType min_epoch = archetype_epoch_;
  EpochManip::setSeq(min_epoch, 1);

  using IntervalType = typename IntegralSet<EpochType>::IntervalType;

  // The allowable interval for this window
  IntervalType interval{min_epoch, max_epoch};

  // The ranged counter for allocating the next epoch which will always be
  // within the interval
  next_epoch_ = std::make_unique<adt::RangedCounter<EpochType>>(
    interval.lower(), interval.upper()
  );

  // All epochs in a given window start out terminated and free (thus, reusable).
  free_epochs_.insertInterval(interval);

  fmt::print(
    "EpochWindow: epoch={:x}, arch={:x}, min={:x}, max={:x}\n",
    epoch, archetype_epoch_, min_epoch, max_epoch
  );

  vt_debug_print(
    term, node,
    "initialize window: epoch={:x}, archetype epoch={:x}\n",
    epoch, archetype_epoch_
  );
}

EpochType EpochWindow::allocateNewEpoch() {
  // Check for the condition where all epochs within the window are active and
  // not terminated and haven't been garbage collected.

  // @todo: need to potentially spin here to garbage collect instead of just
  // aborting!

  vtAbortIf(
    free_epochs_.size() == 0,
    "Must have an epoch to allocate within the specified range"
  );

  // Increment the next epoch counter until we find an terminated epoch that can
  // be allocated

  // @todo: this is probably a little slow, but tries to give a contiguous
  // allocation policy with minimal reuse
  do {
    EpochType next = *next_epoch_;
    if (free_epochs_.contains(next)) {
      (*next_epoch_)++;

      // Tell the system the epoch is now active
      activateEpoch(next);
      return next;
    }
  } while (true);
}

inline bool EpochWindow::isArchetypal(EpochType epoch) {
  auto epoch_arch = epoch;
  epoch::EpochManip::setSeq(epoch_arch,0);
  return epoch_arch == archetype_epoch_;
}

void EpochWindow::activateEpoch(EpochType epoch) {
  vt_debug_print_verbose(
    term, node,
    "activateEpoch: (before) epoch={:x}, first={:x}, last={:x}, num={}, "
    "compression={}\n",
    epoch, free_epochs_.lower(), free_epochs_.upper(),
    free_epochs_.size(), free_epochs_.compression()
  );

  auto is_epoch_arch = isArchetypal(epoch);

  vtAssertExprInfo(
    is_epoch_arch, epoch, is_epoch_arch, archetype_epoch_,
    free_epochs_.lower(), free_epochs_.upper(),
    free_epochs_.size()
  );

  // @todo: I think this is idempotent, thus may be called multiple times

  ///vtAssert(free_epochs_.contains(epoch), "The epoch must be free");

  if (free_epochs_.contains(epoch)) {
    free_epochs_.erase(epoch);
  }

  vt_debug_print(
    term, node,
    "activateEpoch: (after) epoch={:x}, first={:x}, last={:x}, num={}, "
    "compression={}\n",
    epoch, free_epochs_.lower(), free_epochs_.upper(),
    free_epochs_.size(), free_epochs_.compression()
  );
}

void EpochWindow::setEpochTerminated(EpochType epoch) {
  vt_debug_print_verbose(
    term, node,
    "setEpochTerminated: (before) epoch={:x}, first={:x}, last={:x}, num={}, "
    "compression={}\n",
    epoch, terminated_epochs_.lower(), terminated_epochs_.upper(),
    terminated_epochs_.size(), terminated_epochs_.compression()
  );

  terminated_epochs_.insert(epoch);
  total_terminated_++;

  checkGarbageCollection();

  vt_debug_print(
    term, node,
    "setEpochTerminated: (after) epoch={:x}, first={:x}, last={:x}, num={}, "
    "compression={}\n",
    epoch, terminated_epochs_.lower(), terminated_epochs_.upper(),
    terminated_epochs_.size(), terminated_epochs_.compression()
  );

}

void EpochWindow::checkGarbageCollection() {
  if (pending_free_) {
    return;
  }
  pending_free_ = true;

  auto const term_size = terminated_epochs_.size();

  // @todo: make trigger parameterized?

  // If we reach X% of capacity, engage the collection protocol
  if (term_size == next_epoch_->getRange() * 0.10) {
    // At the most, let's say we do 64 MiB of frees at a time. If the
    // compression rate is good, we could do a lot more than that

    auto const max_size = (1 << 26) / (2*sizeof(EpochType));

    vt::IntegralSet<EpochType> to_collect;

    // We can garbage collect the whole thing
    if (terminated_epochs_.compressedSize() < max_size) {
      to_collect = terminated_epochs_;
    } else {
      auto iter = terminated_epochs_.ibegin();
      for (uint64_t i = 0; i < max_size; i++) {
        auto x = *iter;
        to_collect.insertInterval(x);
        iter++;
        vtAssert(
          iter != terminated_epochs_.iend(), "We should not reach the end"
        );
      }
    }

    // start reduction

    NodeType collective_root = 0;

    auto proxy = theEpoch()->getProxy();

    objgroup::proxy::Proxy<EpochManip> p{proxy};

    auto cb = theCB()->makeBcast<
      EpochManip, GarbageCollectMsg, &EpochManip::collectEpochs
    >(p);
    auto msg = makeMessage<GarbageCollectMsg>(archetype_epoch_, to_collect);

    // Get the reducer scope for the epoch component
    auto r = theEpoch()->reducer();

    using collective::reduce::makeStamp;
    using collective::reduce::StrongEpoch;
    using OpType = collective::PlusOp<IntegralSetData>;

    // Stamp this with the epoch's archetype which ensures these don't get mixed
    // up across epoch types garbage collecting concurrently
    auto stamp = makeStamp<StrongEpoch>(archetype_epoch_);
    r->reduce<OpType>(collective_root, msg.get(), cb, stamp);
  }
}

void EpochWindow::garbageCollect(vt::IntegralSet<EpochType> const& eps) {
  for (auto&& x : eps) {
    terminated_epochs_.erase(x);
    free_epochs_.insert(x);
  }

  pending_free_ = false;
}

bool EpochWindow::isTerminated(EpochType epoch) const {
  vt_debug_print(
    term, node,
    "isTerminated: epoch={:x}, first={:x}, last={:x}, num={}, "
    "compression={}\n",
    epoch, terminated_epochs_.lower(), terminated_epochs_.upper(),
    terminated_epochs_.size(), terminated_epochs_.compression()
  );

  // An epoch is terminated if it's known by this node, or has been fully
  // garbage collected
  return terminated_epochs_.contains(epoch) or free_epochs_.contains(epoch);
}

bool EpochWindow::isFree(EpochType epoch) const {
  vt_debug_print(
    term, node,
    "isFree: epoch={:x}, first={:x}, last={:x}, num={}, "
    "compression={}\n",
    epoch, free_epochs_.lower(), free_epochs_.upper(),
    free_epochs_.size(), free_epochs_.compression()
  );

  return free_epochs_.contains(epoch);
}

}} /* end namespace vt::epoch */
