/*
//@HEADER
// *****************************************************************************
//
//                                term_action.cc
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

#include "vt/config.h"
#include "vt/termination/term_action.h"
#include "vt/termination/term_common.h"
#include "vt/termination/termination.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/runnable/make_runnable.h"

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
  auto const status = testEpochTerminated(epoch);
  if (status == TermStatusEnum::Terminated) {
    queueActions(epoch);
  }
}

/*static*/ void TermAction::runActions(ActionMsg* msg) {
  theTerm()->triggerAllEpochActions(msg->ep, msg->encapsulated_epoch);
}

void TermAction::queueActions(EpochType epoch) {
  if (epoch == term::any_epoch_sentinel) {
    // @todo: should this be delayed also?
    for (auto&& action : global_term_actions_) {
      action();
    }

    global_term_actions_.clear();
  } else {
    auto make_runnable = [&](EpochType encap_epoch){
      auto msg = makeMessage<ActionMsg>(epoch, encap_epoch);
      auto const han = auto_registry::makeAutoHandler<ActionMsg, runActions>();
      auto const this_node = theContext()->getNodeStrong();
      runnable::makeRunnable(msg, true, han, this_node)
        .withTDEpoch(encap_epoch)
        .enqueue();
    };

    if (auto iter = epoch_actions_.find(epoch); iter != epoch_actions_.end()) {
      for (auto const& [encapsulated_epoch, _] : iter->second) {
        make_runnable(encapsulated_epoch);
      }
    }

    if (auto iter = epoch_callable_actions_.find(epoch);
        iter != epoch_callable_actions_.end()) {
      for (auto const& [encapsulated_epoch, _] : iter->second) {
        make_runnable(encapsulated_epoch);
      }
    }
  }
}

void TermAction::addActionEpoch(EpochType const& epoch, ActionType action) {
  if (epoch == term::any_epoch_sentinel) {
    return addAction(action);
  } else {
    auto encapsulated_epoch = getCurrentEpoch();
    theTerm()->produce(encapsulated_epoch);
    epoch_actions_[epoch][encapsulated_epoch].push_back(action);
  }
  afterAddEpochAction(epoch);
}

void TermAction::produceOn(EpochType epoch) const {
  theTerm()->produce(epoch);
}

void TermAction::triggerAllEpochActions(
  EpochType epoch, EpochType encapsulated_epoch
) {
  // Run through the normal ActionType elements associated with this epoch
  if (auto iter = epoch_actions_.find(epoch);
      iter != epoch_actions_.end()) {
    if (auto iter2 = iter->second.find(encapsulated_epoch);
        iter2 != iter->second.end()) {
      for (auto&& action : iter2->second) {
        theTerm()->consume(encapsulated_epoch);
        action();
      }
      iter->second.erase(iter2);
    }
    if (iter->second.size() == 0) {
      epoch_actions_.erase(iter);
    }
  }

  // Run through the callables associated with this epoch

  if (auto iter = epoch_callable_actions_.find(epoch);
      iter != epoch_callable_actions_.end()) {
    if (auto iter2 = iter->second.find(encapsulated_epoch);
        iter2 != iter->second.end()) {
      for (auto&& action : iter2->second) {
        theTerm()->consume(encapsulated_epoch);
        action->invoke();
      }
      iter->second.erase(iter2);
    }
    if (iter->second.size() == 0) {
      epoch_callable_actions_.erase(iter);
    }
  }
}

EpochType TermAction::getCurrentEpoch() const {
  return theTerm()->getEpoch();
}

}} /* end namespace vt::term */
