/*
//@HEADER
// *****************************************************************************
//
//                               pending_send.cc
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

#include "vt/messaging/pending_send.h"
#include "vt/messaging/active.h"

namespace vt { namespace messaging {

PendingSend::PendingSend(EpochType ep, EpochActionType const& in_action)
  : epoch_action_{in_action}, epoch_produced_(ep) {
  if (epoch_produced_ != no_epoch) {
    theTerm()->produce(epoch_produced_, 1);
  }
}

PendingSend::PendingSend(PendingSend&& in) noexcept
  : epoch_produced_(std::move(in.epoch_produced_))
{
  std::swap(msg_, in.msg_);
  std::swap(epoch_action_, in.epoch_action_);
  std::swap(send_action_, in.send_action_);
}

void PendingSend::sendMsg() {
  if (send_action_ == nullptr) {
    theMsg()->doMessageSend(msg_);
  } else {
    send_action_(msg_);
  }
  consumeMsg();
  msg_ = nullptr;
  send_action_ = nullptr;
}

EpochType PendingSend::getProduceEpochFromMsg() const {
  if (msg_ == nullptr or envelopeIsTerm(msg_->env) or
      not envelopeIsEpochType(msg_->env)) {
    return no_epoch;
  }

  return envelopeGetEpoch(msg_->env);
}

void PendingSend::produceMsg() {
  epoch_produced_ = getProduceEpochFromMsg();
  if (epoch_produced_ != no_epoch) {
    theTerm()->produce(epoch_produced_, 1);
  }
}

void PendingSend::consumeMsg() {
  if (epoch_produced_ != no_epoch) {
    theTerm()->consume(epoch_produced_, 1);
  }
}

void PendingSend::release() {
  bool send_msg = msg_ != nullptr || send_action_ != nullptr;
  vtAssert(!send_msg || !epoch_action_, "cannot have both a message and epoch action");
  if (send_msg) {
    sendMsg();
  } else if ( epoch_action_ ) {
    epoch_action_();
    epoch_action_ = {};
    consumeMsg();
  }
}

}}
