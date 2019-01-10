/*
//@HEADER
// ************************************************************************
//
//                          term_action.h
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

#if !defined INCLUDED_TERMINATION_TERM_ACTION_H
#define INCLUDED_TERMINATION_TERM_ACTION_H

#include "vt/config.h"
#include "vt/termination/term_common.h"
#include "vt/termination/term_state.h"
#include "vt/termination/term_finished.h"

#include <vector>
#include <unordered_map>

namespace vt { namespace term {

struct TermAction : TermFinished {
  using TermStateType = TermState;
  using ActionContType = std::vector<ActionType>;
  using EpochActionContType = std::unordered_map<EpochType,ActionContType>;
  using EpochStateType = std::unordered_map<EpochType,TermStateType>;

  TermAction() = default;

public:
  void addDefaultAction(ActionType action);
  void addAction(ActionType action);
  void addAction(EpochType const& epoch, ActionType action);
  void addActionEpoch(EpochType const& epoch, ActionType action);
  void clearActions();
  void clearActionsEpoch(EpochType const& epoch);

public:
  /*
   * Deprecated methods for adding a termination action
   */
  [[deprecated("Replaced by `addAction' or `addActionEpoch'")]]
  void attachEpochTermAction(EpochType const& epoch, ActionType action);
  [[deprecated("Replaced by `addAction'")]]
  void attachGlobalTermAction(ActionType action);

protected:
  void triggerAllActions(EpochType const& epoch, EpochStateType const& state);
  void triggerAllEpochActions(EpochType const& epoch);

protected:
  // Container for hold global termination actions
  ActionContType global_term_actions_ = {};
  // Container to hold actions to perform when an epoch has terminated
  EpochActionContType epoch_actions_ = {};
};

}} /* end namespace vt::term */

#endif /*INCLUDED_TERMINATION_TERM_ACTION_H*/
