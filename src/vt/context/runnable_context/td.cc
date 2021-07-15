/*
//@HEADER
// *****************************************************************************
//
//                                    td.cc
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

#include "vt/context/runnable_context/td.h"
#include "vt/termination/termination.h"
#include "vt/messaging/active.h"

namespace vt { namespace ctx {

TD::TD(EpochType in_ep)
  : ep_(in_ep == no_epoch ? term::any_epoch_sentinel : in_ep)
{
  if (ep_ != no_epoch) {
    theTerm()->produce(ep_);
  }
}

/*virtual*/ TD::~TD() {
  if (ep_ != no_epoch) {
    theTerm()->consume(ep_);
  }
}

void TD::begin() {
  theMsg()->pushEpoch(ep_);
  epoch_stack_size_ = theMsg()->getEpochStack().size();
}

void TD::end() {
  auto& epoch_stack = theMsg()->getEpochStack();

  vt_debug_print(
    verbose, context,
    "TD::end: top={:x}, size={}\n",
    epoch_stack.size() > 0 ? epoch_stack.top(): no_epoch,
    epoch_stack.size()
  );

  vtAssertNot(
    epoch_stack_size_ < epoch_stack.size(),
    "Epoch stack popped below preceding push size in handler"
  );

  vtAssert(
    epoch_stack_size_ == epoch_stack.size(), "Stack must be same size"
  );

  vtAssertNotExpr(epoch_stack.size() == 0);

  while (epoch_stack.size() > epoch_stack_size_) {
    theMsg()->popEpoch();
  }

  vtAssertExpr(epoch_stack.size() == epoch_stack_size_);

  theMsg()->popEpoch(ep_);
}

void TD::suspend() {
  auto& epoch_stack = theMsg()->getEpochStack();

  while (epoch_stack.size() > epoch_stack_size_) {
    suspended_epochs_.push_back(theMsg()->getEpoch());
    theMsg()->popEpoch();
  }

  theMsg()->popEpoch(ep_);
}

void TD::resume() {
  auto const sz = suspended_epochs_.size();
  for (std::size_t i = 0; i < sz; i++) {
    theMsg()->pushEpoch(suspended_epochs_[sz - i - 1]);
  }
  suspended_epochs_.clear();

  theMsg()->pushEpoch(ep_);
}

}} /* end namespace vt::ctx */
