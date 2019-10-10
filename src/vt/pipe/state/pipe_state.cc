/*
//@HEADER
// *****************************************************************************
//
//                                pipe_state.cc
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
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/state/pipe_state.h"

namespace vt { namespace pipe {

PipeState::PipeState(
  PipeType const& in_pipe, RefType const& in_signals, RefType const& in_lis,
  bool const& in_typeless
) : automatic_(true), typeless_(in_typeless), num_signals_expected_(in_signals),
    num_listeners_expected_(in_lis), pipe_(in_pipe)
{}

PipeState::PipeState(PipeType const& in_pipe, bool const& in_typeless)
  : automatic_(false), typeless_(in_typeless), pipe_(in_pipe)
{}

void PipeState::signalRecv() {
  num_signals_received_++;
}

void PipeState::listenerReg() {
  num_listeners_received_++;
}

bool PipeState::isAutomatic() const {
  return automatic_;
}

bool PipeState::isTypeless() const {
  return typeless_;
}

bool PipeState::isPersist() const {
  return !automatic_;
}

PipeType PipeState::getPipe() const {
  return pipe_;
}

RefType PipeState::refsPerListener() const {
  return num_signals_expected_;
}

bool PipeState::hasDispatch() const {
  return dispatch_ != nullptr;
}

void PipeState::setDispatch(DispatchFuncType in_dispatch) {
  dispatch_ = in_dispatch;
}

void PipeState::dispatch(void* ptr) {
  vtAssert(dispatch_ != nullptr, "Dispatch function must be available");
  dispatch_(ptr);
}

bool PipeState::finished() const {
  if (automatic_) {
    auto const total_expect = num_signals_expected_ * num_listeners_expected_;
    auto const total_recv = num_signals_received_ * num_listeners_received_;
    return total_expect == total_recv;
  } else {
    return false;
  }
}

}} /* end namespace vt::pipe */
