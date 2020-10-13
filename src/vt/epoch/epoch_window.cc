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

  // All epochs in a given window start out terminated (thus, reusable).
  terminated_epochs_.insertInterval(interval);

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
  // Check for a strange edge condition where all epochs within the window
  // are active and not terminated.
  vtAbortIf(
    terminated_epochs_.size() == 0,
    "Must have an epoch to allocate within the specified range"
  );

  // Increment the next epoch counter until we find an terminated epoch that can
  // be allocated
  do {
    EpochType next = *next_epoch_;
    if (terminated_epochs_.contains(next)) {
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
    epoch, terminated_epochs_.lower(), terminated_epochs_.upper(),
    terminated_epochs_.size(), terminated_epochs_.compression()
  );

  auto is_epoch_arch = isArchetypal(epoch);

  vtAssertExprInfo(
    is_epoch_arch, epoch, is_epoch_arch, archetype_epoch_,
    terminated_epochs_.lower(), terminated_epochs_.upper(),
    terminated_epochs_.size()
  );

  vtAssert(terminated_epochs_.contains(epoch), "Epoch must be terminated");

  terminated_epochs_.erase(epoch);

  vt_debug_print(
    term, node,
    "activateEpoch: (after) epoch={:x}, first={:x}, last={:x}, num={}, "
    "compression={}\n",
    epoch, terminated_epochs_.lower(), terminated_epochs_.upper(),
    terminated_epochs_.size(), terminated_epochs_.compression()
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

  vt_debug_print(
    term, node,
    "setEpochTerminated: (after) epoch={:x}, first={:x}, last={:x}, num={}, "
    "compression={}\n",
    epoch, terminated_epochs_.lower(), terminated_epochs_.upper(),
    terminated_epochs_.size(), terminated_epochs_.compression()
  );
}

bool EpochWindow::isTerminated(EpochType epoch) const {
  vt_debug_print(
    term, node,
    "isTerminated: epoch={:x}, first={:x}, last={:x}, num={}, "
    "compression={}\n",
    epoch, terminated_epochs_.lower(), terminated_epochs_.upper(),
    terminated_epochs_.size(), terminated_epochs_.compression()
  );

  return terminated_epochs_.contains(epoch);
}

}} /* end namespace vt::epoch */
