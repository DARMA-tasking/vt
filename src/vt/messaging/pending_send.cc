/*
//@HEADER
// *****************************************************************************
//
//                               pending_send.cc
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

#include "vt/messaging/pending_send.h"
#include "vt/messaging/active.h"

namespace vt { namespace messaging {

void PendingSend::sendMsg() {
  if (send_action_ == nullptr) {
    theMsg()->sendMsgSized(msg_, msg_size_);
  } else {
    send_action_(msg_);
  }
  produceConsumeMsg(PendingTermEnum::Consume);
  msg_ = nullptr;
  send_action_ = nullptr;
}

void PendingSend::produceConsumeMsg(PendingTermEnum op) {
  if (msg_ != nullptr) {
    auto const is_epoch = envelopeIsEpochType(msg_->env);
    auto const is_term = envelopeIsTerm(msg_->env);
    if (is_epoch and not is_term) {
      bool const is_produce = op == PendingTermEnum::Produce;
      auto const ep = is_produce ? envelopeGetEpoch(msg_->env) : epoch_produced_;
      if (ep != no_epoch and ep != term::any_epoch_sentinel) {
        if (is_produce) {
          theTerm()->produce(ep,1);
          epoch_produced_ = ep;
        } else {
          theTerm()->consume(ep,1);
        }
      }
    }
  }
}

}}
