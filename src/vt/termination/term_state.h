/*
//@HEADER
// ************************************************************************
//
//                          term_state.h
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

#if !defined INCLUDED_TERMINATION_TERM_STATE_H
#define INCLUDED_TERMINATION_TERM_STATE_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/termination/term_common.h"
#include "vt/termination/term_parent.h"

#include <vector>
#include <cstdlib>
#include <cassert>

namespace vt { namespace term {

struct TermState : EpochRelation {
  using EventCountType = int32_t;

  void notifyChildReceive();
  bool isTerminated() const;
  void setTerminated();
  void activateEpoch();
  void notifyLocalTerminated(bool const terminated = true);
  void submitToParent(bool const is_root, bool const setup = false);
  void receiveContinueSignal(TermWaveType const& wave);
  bool readySubmitParent(bool const needs_active = true) const;
  EventCountType getRecvChildCount() const;
  EpochType getEpoch() const;
  TermWaveType getCurWave() const;
  void setCurWave(TermWaveType const& wave);
  NodeType getNumChildren() const;
  bool noLocalUnits() const;

  TermState(
    EpochType const& in_epoch, bool const in_local_terminated, bool const active,
    NodeType const& children
  );
  TermState(EpochType const& in_epoch, NodeType const& children);

  TermState(TermState const&) = default;
  TermState(TermState&&) = default;
  TermState& operator=(TermState const&) = default;

  // four-counter method (local prod/cons, global prod/cons 1/2)
  TermCounterType l_prod                      = 0;
  TermCounterType l_cons                      = 0;
  TermCounterType g_prod1                     = 0;
  TermCounterType g_cons1                     = 0;
  TermCounterType g_prod2                     = 0;
  TermCounterType g_cons2                     = 0;
  EventCountType  constant_count              = 0;
  EventCountType  num_print_constant          = 0;

private:
  // Boolean local_terminated is for future optimization to disable propagation
  // when its known that global termination is impossible because locally the
  // system has not terminated
  bool local_terminated_                      = true;
  bool epoch_active_                          = true;
  bool term_detected_                         = false;

  EventCountType recv_child_count_            = 0;
  NodeType num_children_                      = uninitialized_destination;
  TermWaveType cur_wave_                      = 0;
  TermWaveType  submitted_wave_               = -1;
};

}} //end namespace vt::term

#endif /*INCLUDED_TERMINATION_TERM_STATE_H*/
