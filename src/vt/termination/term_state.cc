/*
//@HEADER
// ************************************************************************
//
//                          term_state.cc
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
#include "vt/termination/term_state.h"
#include "vt/termination/termination.h"

namespace vt { namespace term {

TermWaveType TermState::getCurWave() const {
  return cur_wave_;
}

void TermState::setCurWave(TermWaveType const& wave) {
  cur_wave_ = wave;
}

NodeType TermState::getNumChildren() const {
  return num_children_;
}

EpochType TermState::getEpoch() const {
  return epoch_;
}

TermState::EventCountType TermState::getRecvChildCount() const {
  return recv_child_count_;
}

void TermState::notifyChildReceive() {
  recv_child_count_++;

  debug_print_verbose(
    term, node,
    "notifyChildReceive: epoch={:x}, active={}, local_ready={}, "
    "submitted_wave={}, recv={}, children={}\n",
    epoch_, print_bool(epoch_active_), print_bool(local_terminated_),
    submitted_wave_, recv_child_count_, num_children_
  );

  vtAssert(recv_child_count_ <= num_children_, "Must be <= than num children");
}

bool TermState::noLocalUnits() const {
  return l_prod == 0 && l_cons == 0;
}

void TermState::setTerminated() {
  term_detected_ = true;
}

bool TermState::isTerminated() const {
  return term_detected_;
}

void TermState::activateEpoch() {
  epoch_active_ = true;
}

void TermState::notifyLocalTerminated(bool const terminated) {
  local_terminated_ = terminated;
}

void TermState::submitToParent(bool const, bool const setup) {
  if (not setup) {
    submitted_wave_++;
  }
  recv_child_count_ = 0;
}

void TermState::receiveContinueSignal(TermWaveType const& wave) {
  vtAssert(cur_wave_ == wave - 1, "Wave must monotonically increase");
  cur_wave_ = wave;
}

bool TermState::readySubmitParent(bool const needs_active) const {
  vtAssert(
    num_children_ != uninitialized_destination, "Children must be valid"
  );

  auto const ret = (epoch_active_ or not needs_active) and
    recv_child_count_ == num_children_ and local_terminated_ and
    submitted_wave_ == cur_wave_ - 1 and not term_detected_;

  debug_print_verbose(
    term, node,
    "readySubmitParent: epoch={:x}, active={}, local_ready={}, "
    "sub_wave={}, cur_wave_={}, recv_child={}, num_child={}, term={}:"
    " ret={}\n",
    epoch_, print_bool(epoch_active_), print_bool(local_terminated_),
    submitted_wave_, cur_wave_, recv_child_count_, num_children_,
    print_bool(term_detected_), print_bool(ret)
  );

  return ret;
}

TermState::TermState(
  EpochType const& in_epoch, bool const in_local_terminated, bool const active,
  NodeType const& children
)
  : EpochRelation(in_epoch, false),
    local_terminated_(in_local_terminated), epoch_active_(active),
    num_children_(children)
{
  debug_print(
    term, node,
    "TermState: constructor: epoch={:x}, num_children={}, active={}, "
    "local_terminated={}\n",
    epoch_, num_children_, print_bool(epoch_active_), print_bool(local_terminated_)
  );
}

TermState::TermState(EpochType const& in_epoch, NodeType const& children)
  : EpochRelation(in_epoch, false), num_children_(children)
{
  debug_print(
    term, node,
    "TermState: constructor: epoch={:x}, event={}\n", epoch_, recv_child_count_
  );
}

}} /* end namespace vt::term */
