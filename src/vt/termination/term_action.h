/*
//@HEADER
// *****************************************************************************
//
//                                term_action.h
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

#if !defined INCLUDED_VT_TERMINATION_TERM_ACTION_H
#define INCLUDED_VT_TERMINATION_TERM_ACTION_H

#include "vt/config.h"
#include "vt/termination/term_common.h"
#include "vt/termination/term_state.h"
#include "vt/termination/term_terminated.h"

#include <vector>
#include <unordered_map>
#include <memory>

namespace vt { namespace term {

struct CallableBase {
protected:
  CallableBase() = default;
public:
  CallableBase(CallableBase const&) = delete;
  CallableBase(CallableBase&&) = default;
  virtual ~CallableBase() = default;
  virtual void invoke() = 0;
};

template <typename Callable>
struct CallableHolder : CallableBase {
  explicit CallableHolder(Callable&& in_c) : c_(std::move(in_c)) { }
  CallableHolder(CallableHolder const&) = delete;
  CallableHolder(CallableHolder&&) = default;

protected:
  constexpr Callable&& move() { return std::move(c_); }

  template <typename... A>
  auto operator()(A&&... a) -> decltype(auto) {
    return c_(std::forward<A>(a)...);
  }

public:
  virtual void invoke() override {
    // Skipping arguments for now (ActionType use case)
    this->operator()();
  }

private:
  Callable c_;
};


struct TermAction : TermTerminated {
  using TermStateType         = TermState;
  using ActionContType        = std::vector<ActionType>;
  using CallableActionType    = std::unique_ptr<CallableBase>;
  using CallableVecType       = std::vector<CallableActionType>;
  using CallableContType      = std::unordered_map<EpochType,CallableVecType>;
  using EpochActionContType   = std::unordered_map<EpochType,ActionContType>;
  using EpochStateType        = std::unordered_map<EpochType,TermStateType>;

  TermAction() = default;

public:
  void addDefaultAction(ActionType action);
  void addAction(ActionType action);
  void addAction(EpochType const& epoch, ActionType action);
  void addActionEpoch(EpochType const& epoch, ActionType action);
  void clearActions();
  void clearActionsEpoch(EpochType const& epoch);

  template <typename Callable>
  void addActionUnique(EpochType const& epoch, Callable&& c);

protected:
  void triggerAllActions(EpochType const& epoch);
  void triggerAllEpochActions(EpochType const& epoch);
  void afterAddEpochAction(EpochType const& epoch);

protected:
  // Container for hold global termination actions
  ActionContType global_term_actions_ = {};
  // Container to hold actions to perform when an epoch has terminated
  EpochActionContType epoch_actions_ = {};
  // Container for "callables"; restricted in semantic wrt std::function
  CallableContType epoch_callable_actions_ = {};
};

}} /* end namespace vt::term */

#include "vt/termination/term_action.impl.h"

#endif /*INCLUDED_VT_TERMINATION_TERM_ACTION_H*/
