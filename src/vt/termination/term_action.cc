/*
//@HEADER
// *****************************************************************************
//
//                                term_action.cc
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
#include "vt/termination/term_action.h"
#include "vt/termination/term_common.h"
#include "vt/termination/termination.h"

namespace vt { namespace term {

void TermAction::addDefaultAction(ActionType action) {
  return addAction(action);
}

void TermAction::addAction(ActionType action) {
  global_term_actions_.emplace_back(action);
}

void TermAction::addAction(EpochType const& epoch, ActionType action) {
  return addActionEpoch(epoch,action);
}

void TermAction::afterAddEpochAction(EpochType const& epoch) {
  /*
   *  Produce a unit of any epoch type to inhibit global termination when
   *  local termination of a specific epoch is waiting for detection
   */
  theTerm()->produce(term::any_epoch_sentinel);

  auto const status = testEpochTerminated(epoch);
  if (status == TermStatusEnum::Terminated) {
    triggerAllEpochActions(epoch);
  }
}

void TermAction::addActionEpoch(EpochType const& epoch, ActionType action) {
  if (epoch == term::any_epoch_sentinel) {
    return addAction(action);
  } else {
    auto epoch_iter = epoch_actions_.find(epoch);
    if (epoch_iter == epoch_actions_.end()) {
      epoch_actions_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(epoch),
        std::forward_as_tuple(ActionContType{action})
      );
    } else {
      epoch_iter->second.emplace_back(action);
    }
  }
  afterAddEpochAction(epoch);
}

void TermAction::clearActions() {
  global_term_actions_.clear();
}

void TermAction::clearActionsEpoch(EpochType const& epoch) {
  if (epoch == term::any_epoch_sentinel) {
    return clearActions();
  } else {
    auto iter = epoch_actions_.find(epoch);
    if (iter != epoch_actions_.end()) {
      auto const& epoch_actions_count = iter->second.size();
      epoch_actions_.erase(iter);
      /*
       *  Consume units of epoch-specific actions are cleared to match the
       *  production in addActionEpoch
       */
      theTerm()->consume(term::any_epoch_sentinel, epoch_actions_count);
    }
  }
}

void TermAction::triggerAllActions(EpochType const& epoch) {
  if (epoch == term::any_epoch_sentinel) {
    for (auto&& action : global_term_actions_) {
      action();
    }

    global_term_actions_.clear();
  } else {
    return triggerAllEpochActions(epoch);
  }
}

void TermAction::triggerAllEpochActions(EpochType const& epoch) {
  // Run through the normal ActionType elements associated with this epoch
  std::size_t epoch_actions_count = 0;
  auto iter = epoch_actions_.find(epoch);
  if (iter != epoch_actions_.end()) {
    epoch_actions_count += iter->second.size();
    for (auto&& action : iter->second) {
      action();
    }
    epoch_actions_.erase(iter);
  }
  // Run through the callables associated with this epoch
  auto iter2 = epoch_callable_actions_.find(epoch);
  if (iter2 != epoch_callable_actions_.end()) {
    epoch_actions_count += iter2->second.size();

    for (auto&& action : iter2->second) {
      action->invoke();
    }

    epoch_callable_actions_.erase(iter2);
  }
  /*
   *  Consume number of action units of any epoch type to match the production
   *  in addActionEpoch() so global termination can now be detected
   */
  theTerm()->consume(term::any_epoch_sentinel, epoch_actions_count);
}

}} /* end namespace vt::term */
