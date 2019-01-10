/*
//@HEADER
// ************************************************************************
//
//                          term_window.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include "vt/config.h"
#include "vt/termination/term_window.h"

namespace vt { namespace term {

void EpochWindow::initialize(EpochType const& epoch) {
  if (conform_archetype_) {
    vtAssertExpr(archetype_epoch_ == no_epoch);
    vtAssertExpr(!initialized_);
    if (archetype_epoch_ == no_epoch) {
      auto arch_epoch = epoch;
      /*
       *  Set the sequence to zero so the archetype can be compared easily to
       *  incoming epochs to check that they match all the fields. The window
       *  relies on the sequentiality of same-typed epochs to create a
       *  resolved/unresolved window of open epochs.
       */
      epoch::EpochManip::setSeq(arch_epoch,0);
      archetype_epoch_ = arch_epoch;
      initialized_ = true;
    }
  } else {
    vtAssertExpr(conform_archetype_ && initialized_);
  }
}

inline bool EpochWindow::isArchetypal(EpochType const& epoch) {
  auto epoch_arch = epoch;
  epoch::EpochManip::setSeq(epoch_arch,0);
  return epoch_arch == archetype_epoch_;
}

bool EpochWindow::inWindow(EpochType const& epoch) const {
  vtAssertExpr(last_unresolved_epoch_ >= first_unresolved_epoch_);
  vtAssertExpr(initialized_);
  return epoch < first_unresolved_epoch_;
}

void EpochWindow::addEpoch(EpochType const& epoch) {
  debug_print(
    term, node,
    "addEpoch: (before) epoch={:x}, unresolved: first={:x}, last={:x}\n",
    epoch, first_unresolved_epoch_, last_unresolved_epoch_
  );

  if (!initialized_) {
    initialize(epoch);
  }

  if (conform_archetype_) {
    auto is_epoch_arch = isArchetypal(epoch);

    vtAssertExprInfo(
      is_epoch_arch, epoch, is_epoch_arch, archetype_epoch_,
      initialized_, first_unresolved_epoch_, last_unresolved_epoch_,
      finished_.size()
    );
  }

  if (first_unresolved_epoch_ == no_epoch) {
    vtAssertExpr(last_unresolved_epoch_ == no_epoch);
    /*
     * Set the first and last epoch, since this is the first time this window
     * is being initialized for the given epoch configuration
     */
    first_unresolved_epoch_ = last_unresolved_epoch_ = epoch;
  } else {
    last_unresolved_epoch_ = std::max(last_unresolved_epoch_, epoch);
  }

  vtAssertExpr(last_unresolved_epoch_ >= first_unresolved_epoch_);

  debug_print(
    term, node,
    "addEpoch: (after) epoch={:x}, unresolved: first={:x}, last={:x}\n",
    epoch, first_unresolved_epoch_, last_unresolved_epoch_
  );
}

void EpochWindow::closeEpoch(EpochType const& epoch) {
  debug_print(
    term, node,
    "closeEpoch: (before) epoch={:x}, unresolved: first={:x}, last={:x}\n",
    epoch, first_unresolved_epoch_, last_unresolved_epoch_
  );

  /*
   * If this is the first_unresolved_epoch_, i.e., the epoch is resolved
   * sequentially wrt to the other similarity-typed epochs, then we just
   * increment the first
   */
  bool insert_into_finished = true;

  if (first_unresolved_epoch_ == epoch) {
    debug_print(
      term, node,
      "closeEpoch: epoch={:x}, unresolved: first={:x}, last={:x}, inc\n",
      epoch, first_unresolved_epoch_, last_unresolved_epoch_
    );

    /*
     *  Do not insert because it is within the new resolved window and thus
     *  all finished operations will be complete
     */
    insert_into_finished = false;

    first_unresolved_epoch_++;
    if (last_unresolved_epoch_ == first_unresolved_epoch_ - 1) {
      last_unresolved_epoch_ = first_unresolved_epoch_;
    }
    vtAssertExpr(last_unresolved_epoch_ >= first_unresolved_epoch_);

    /*
     * Transitively move out-of-order finished epochs out of the finished
     * container as the unresolved epoch windows is closed
     */
    if (finished_.size() > 0) {
      auto iter = finished_.begin();
      while (iter != finished_.end() && *iter == first_unresolved_epoch_) {
        debug_print(
          term, node,
          "closeEpoch: epoch={:x}, unresolved: first={:x}, last={:x}:"
          "inc while: found finished epoch={:x}\n",
          epoch, first_unresolved_epoch_, last_unresolved_epoch_,
          *iter
        );

        first_unresolved_epoch_++;
        iter = finished_.erase(iter);
      }
    }
  }

  if (insert_into_finished) {
    finished_.insert(epoch);
  }

  debug_print(
    term, node,
    "closeEpoch: (after) epoch={:x}, unresolved: first={:x}, last={:x}\n",
    epoch, first_unresolved_epoch_, last_unresolved_epoch_
  );
}

bool EpochWindow::isFinished(EpochType const& epoch) const {
  auto const in_window = inWindow(epoch);

  debug_print(
    term, node,
    "isFinished: epoch={:x}, first={:x}, last={:x}, in_window={}\n",
    epoch, first_unresolved_epoch_, last_unresolved_epoch_,
    in_window
  );

  if (in_window) {
    return true;
  } else {
    auto iter = finished_.find(epoch);
    return iter != finished_.end();
  }
}

void EpochWindow::clean(EpochType const& epoch) {
  auto iter = finished_.find(epoch);
  if (iter != finished_.end()) {
    finished_.erase(iter);
  }
}

}} /* end namespace vt::term */
