/*
//@HEADER
// *****************************************************************************
//
//                              termination.impl.h
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

#if !defined INCLUDED_VT_TERMINATION_TERMINATION_IMPL_H
#define INCLUDED_VT_TERMINATION_TERMINATION_IMPL_H

#include "vt/config.h"
#include "vt/termination/termination.h"
#include "vt/termination/term_common.h"
#include "vt/epoch/epoch_manip.h"

namespace vt { namespace term {

inline void TerminationDetector::produce(
  EpochType epoch, TermCounterType num_units, NodeType node
) {
  vt_debug_print(verbose, term, "produce: epoch={:x}, node={}\n", epoch, node);
  auto const in_epoch = epoch == no_epoch ? any_epoch_sentinel : epoch;
  return produceConsume(in_epoch, num_units, true, node);
}

inline void TerminationDetector::consume(
  EpochType epoch, TermCounterType num_units, NodeType node
) {
  vt_debug_print(verbose, term, "consume: epoch={:x}, node={}\n", epoch, node);
  auto const in_epoch = epoch == no_epoch ? any_epoch_sentinel : epoch;
  return produceConsume(in_epoch, num_units, false, node);
}

/*static*/ inline bool TerminationDetector::isRooted(EpochType epoch) {
  bool const is_sentinel = epoch == any_epoch_sentinel or epoch == no_epoch;
  return is_sentinel ? false : epoch::EpochManip::isRooted(epoch);
}

/*static*/ inline bool TerminationDetector::isDS(EpochType epoch) {
  return epoch::EpochManip::isDS(epoch);
}

/*static*/ inline bool TerminationDetector::isDep(EpochType epoch) {
  return epoch::EpochManip::isDep(epoch);
}

inline void TerminationDetector::produceConsumeState(
  TermStateType& state, TermCounterType const num_units, bool produce
) {
  auto& counter = produce ? state.l_prod : state.l_cons;
  counter += num_units;

  vt_debug_print(
    verbose, term,
    "produceConsumeState: epoch={:x}, event_count={}, l_prod={}, l_cons={}, "
    "num_units={}, produce={}\n",
    state.getEpoch(), state.getRecvChildCount(), state.l_prod, state.l_cons, num_units,
    print_bool(produce)
  );

  if (state.isActive() and state.readySubmitParent()) {
    propagateEpoch(state);
  }
}

inline void TerminationDetector::produceConsume(
  EpochType epoch, TermCounterType num_units, bool produce, NodeType node
) {
  vt_debug_print(
    normal, term,
    "produceConsume: epoch={:x}, rooted={}, ds={}, count={}, produce={}, "
    "node={}\n",
    epoch, isRooted(epoch), isDS(epoch), num_units, produce, node
  );

  produceConsumeState(any_epoch_state_, num_units, produce);

  if (epoch != any_epoch_sentinel) {
    if (isDS(epoch)) {
      auto ds_term = getDSTerm(epoch);

      // If a node is not passed, use the current node (self-prod/cons)
      if (node == uninitialized_destination) {
        node = this_node_;
      }

      if (produce) {
        ds_term->msgSent(node,num_units);
      } else {
        ds_term->msgProcessed(node,num_units);
      }
    } else {
      auto& state = findOrCreateState(epoch, false);
      produceConsumeState(state, num_units, produce);
    }
  }
}

inline EpochType TerminationDetector::getEpoch() const {
  vtAssertInfo(
    epoch_stack_.size() > 0, "Epoch stack size must be greater than zero",
    epoch_stack_.size()
  );
  return epoch_stack_.size() ? EpochType{epoch_stack_.top()} : term::any_epoch_sentinel;
}
 inline void TerminationDetector::pushEpoch(EpochType epoch) {
  /*
   * pushEpoch(epoch) pushes any epoch onto the local stack iff epoch !=
   * no_epoch; the epoch stack includes all locally pushed epochs and the
   * current contexts pushed, transitively causally related active message
   * handlers.
   */
  vtAssertInfo(
    epoch != no_epoch, "Do not push no_epoch onto the epoch stack",
    epoch, no_epoch, epoch_stack_.size(),
    epoch_stack_.size() > 0 ? EpochType{epoch_stack_.top()} : no_epoch
  );
  if (epoch != no_epoch) {
    epoch_stack_.push(epoch.get());
  }
}

inline EpochType TerminationDetector::popEpoch(EpochType epoch) {
  /*
   * popEpoch(epoch) shall remove the top entry from epoch_size_, iif the size
   * is non-zero and the `epoch' passed, if `epoch != no_epoch', is equal to the
   * top of the `epoch_stack_.top()'; else, it shall remove any entry from the
   * top of the stack.
   */
  auto const& non_zero = epoch_stack_.size() > 0;
  vtAssertExprInfo(
    non_zero and (epoch_stack_.top() == epoch.get() or epoch == no_epoch),
    epoch, non_zero, non_zero ? EpochType{epoch_stack_.top()} : no_epoch
  );
  if (epoch == no_epoch) {
    return non_zero ? epoch_stack_.pop(),EpochType{epoch_stack_.top()} : no_epoch;
  } else {
    return non_zero && epoch == EpochType{epoch_stack_.top()} ?
      epoch_stack_.pop(),epoch :
      no_epoch;
  }
}


}} /* end namespace vt::term */

#endif /*INCLUDED_VT_TERMINATION_TERMINATION_IMPL_H*/
