/*
//@HEADER
// *****************************************************************************
//
//                                 term_msgs.h
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

#if !defined INCLUDED_TERMINATION_TERM_MSGS_H
#define INCLUDED_TERMINATION_TERM_MSGS_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/termination/term_state.h"

namespace vt { namespace term {

struct TermMsg : vt::ShortMessage {
  EpochType new_epoch = no_epoch;
  TermCounterType wave = -1;

  explicit TermMsg(EpochType const& in_new_epoch, TermCounterType in_wave = -1)
    : ShortMessage(), new_epoch(in_new_epoch), wave(in_wave)
  { }
};

struct TermTerminatedMsg : vt::Message {

  TermTerminatedMsg() = default;
  TermTerminatedMsg(EpochType const& in_epoch, NodeType const& in_from_node)
    : epoch_(in_epoch), from_node_(in_from_node)
  { }

  EpochType getEpoch() const { return epoch_; }
  NodeType getFromNode() const { return from_node_; }

private:
  EpochType epoch_    = no_epoch;
  NodeType from_node_ = uninitialized_destination;
};

struct TermTerminatedReplyMsg : vt::Message {

  TermTerminatedReplyMsg() = default;
  TermTerminatedReplyMsg(EpochType const& in_epoch, bool const& in_finished)
    : epoch_(in_epoch), finished_(in_finished)
  { }

  EpochType getEpoch() const { return epoch_; }
  bool isTerminated() const { return finished_; }

private:
  EpochType epoch_ = no_epoch;
  bool finished_   = false;
};

struct HangCheckMsg : vt::ShortMessage { };

struct TermCounterMsg : vt::ShortMessage {
  EpochType epoch = no_epoch;
  TermCounterType prod = 0, cons = 0;

  TermCounterMsg(
    EpochType const in_epoch,
    TermCounterType const in_prod, TermCounterType const in_cons
  ) : ShortMessage(), epoch(in_epoch), prod(in_prod), cons(in_cons)
  { }
};

struct BuildGraphMsg : vt::ShortMessage { };

}} //end namespace vt::term

#endif /*INCLUDED_TERMINATION_TERM_MSGS_H*/
