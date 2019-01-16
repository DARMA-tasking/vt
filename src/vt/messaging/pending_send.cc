/*
//@HEADER
// ************************************************************************
//
//                          pending_send.cc
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
#include "vt/messaging/active.h"
#include "vt/messaging/pending_send.h"

namespace vt { namespace messaging {

PendingSend::~PendingSend() {
  release();
}

void PendingSend::release() {
  if (msg_ != nullptr || send_action_ != nullptr) {
    vtAssert(
      epoch_ == no_epoch, "PendingSend should not have an epoch and a message"
    );
    sendMsg();
  }
}

EpochType PendingSend::getEpoch() {
  if (epoch_ != no_epoch) {
    return epoch_;
  }
  if (msg_ != nullptr) {
    auto is_epoch_msg = envelopeIsEpochType(msg_->env);
    vtAbortIf(!is_epoch_msg, "Message must be able to hold an epoch");
    createEpoch();
    return epoch_;
  } else {
    vtAbort("Tried to get an epoch while not holding a valid message");
  }
  return no_epoch;
}

void PendingSend::createEpoch() {
  vtAssert(
    msg_ != nullptr || send_action_ != nullptr,
    "Invariant that PendingSend hold message ^ epoch failed "
  );
  auto cur_msg_epoch = envelopeGetEpoch(msg_->env);
  if (cur_msg_epoch == no_epoch) {
    epoch_ = theTerm()->makeEpochRooted();
    theMsg()->setEpochMessage(msg_, epoch_);
  } else {
    epoch_ = cur_msg_epoch;
  }
  sendMsg();
}

void PendingSend::sendMsg() {
  // Send the message saved directly or trigger the lambda for specialized sends
  // from the pending holder
  if (send_action_ == nullptr) {
    theMsg()->sendMsgSized(msg_, msg_size_);
  } else {
    send_action_(msg_);
  }
  msg_ = nullptr;
  send_action_ = nullptr;
}

void PendingSend::handlePresetOrNoEpoch() {
  if (envelopeIsEpochType(msg_->env)) {
    // If there is an epoch already assigned, save it in the holder and send the
    // message off
    auto cur_msg_epoch = envelopeGetEpoch(msg_->env);
    if (cur_msg_epoch != no_epoch) {
      epoch_ = cur_msg_epoch;
      sendMsg();
    }
  } else {
    // If the message does not hold an epoch (control msg), send it immediately
    sendMsg();
  }
}

}} /* end namespace vt::messaging */
