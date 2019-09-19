/*
//@HEADER
// *****************************************************************************
//
//                                term_window.cc
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
#include "vt/termination/term_window.h"

namespace vt { namespace term {

EpochWindow::EpochWindow(EpochType const& epoch)
  : terminated_epochs_(epoch)
 {
  auto arch_epoch = epoch;
  /*
   *  Set the sequence to zero so the archetype can be compared easily to
   *  incoming epochs to check that they match all the fields. The window
   *  relies on the sequentiality of same-typed epochs to create a
   *  resolved/unresolved window of open epochs.
   */
  epoch::EpochManip::setSeq(arch_epoch,0);
  archetype_epoch_ = arch_epoch;

  vtAssertExpr(epoch == archetype_epoch_);

  debug_print(
    term, node,
    "initialize window: epoch={:x}, archetype epoch={:x}\n",
    epoch, archetype_epoch_
  );
}

inline bool EpochWindow::isArchetypal(EpochType const& epoch) {
  auto epoch_arch = epoch;
  epoch::EpochManip::setSeq(epoch_arch,0);
  return epoch_arch == archetype_epoch_;
}

void EpochWindow::addEpoch(EpochType const& epoch) {
  debug_print_verbose(
    term, node,
    "addEpoch: (before) epoch={:x}, first={:x}, last={:x}, num={}, "
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

  // We should possibly perform some error checking once we wrap around in case
  // of pending actions
  if (terminated_epochs_.contains(epoch)) {
    terminated_epochs_.erase(epoch);
  }

  debug_print(
    term, node,
    "addEpoch: (after) epoch={:x}, first={:x}, last={:x}, num={}, "
    "compression={}\n",
    epoch, terminated_epochs_.lower(), terminated_epochs_.upper(),
    terminated_epochs_.size(), termin_.compression()
  );
}

void EpochWindow::closeEpoch(EpochType const& epoch) {
  debug_print_verbose(
    term, node,
    "closeEpoch: (before) epoch={:x}, first={:x}, last={:x}, num={}, "
    "compression={}\n",
    epoch, terminated_epochs_.lower(), terminated_epochs_.upper(),
    terminated_epochs_.size(), terminated_epochs_.compression()
  );

  terminated_epochs_.insert(epoch);

  debug_print(
    term, node,
    "closeEpoch: (after) epoch={:x}, first={:x}, last={:x}, num={}, "
    "compression={}\n",
    epoch, terminated_epochs_.lower(), terminated_epochs_.upper(),
    terminated_epochs_.size(), terminated_epochs_.compression()
  );
}

bool EpochWindow::isTerminated(EpochType const& epoch) const {
  debug_print(
    term, node,
    "isTerminated: epoch={:x}, first={:x}, last={:x}, num={}, "
    "compression={}\n",
    epoch, terminated_epochs_.lower(), terminated_epochs_.upper(), term_.size(),
    terminated_epochs_.compression()
  );

  return terminated_epochs_.contains(epoch);
}

}} /* end namespace vt::term */
