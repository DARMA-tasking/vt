/*
//@HEADER
// *****************************************************************************
//
//                            dependent_send_chain.h
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

#if !defined INCLUDED_VT_MESSAGING_DEPENDENT_SEND_CHAIN_H
#define INCLUDED_VT_MESSAGING_DEPENDENT_SEND_CHAIN_H

#include "vt/config.h"
#include "vt/messaging/pending_send.h"
#include "vt/epoch/epoch.h"

#include <functional>

namespace vt { namespace messaging {

struct PendingClosure {
  explicit PendingClosure(PendingSend&& in_pending)
    : pending_(std::move(in_pending))
  { }
  PendingClosure(PendingClosure const&) = delete;
  PendingClosure(PendingClosure&& in) = default;

  void operator()() {
    pending_.release();
  }

private:
  PendingSend pending_;
};

class DependentSendChain final {
 public:
  DependentSendChain() { }

  // Add a task to the chain of work to be run in the specified epoch,
  // with subsequent tasks dependent on all work occuring in the
  // specified epoch
  void add(EpochType new_epoch, PendingSend&& link) {
    checkInit();

    // Produce an event here, to be consumed when the send gets
    // released, so that new_epoch doesn't appear to have completed in
    // the interim
    theTerm()->addDependency(last_epoch_, new_epoch);
    theTerm()->addActionUnique(last_epoch_, PendingClosure(std::move(link)));

    last_epoch_ = new_epoch;
  }

  void done() {
    reset();
  }

  bool isTerminated() {
    return theTerm()->testEpochTerminated(last_epoch_) == vt::term::TermStatusEnum::Terminated;
  }

  DependentSendChain            (const DependentSendChain&) = delete;
  DependentSendChain            (DependentSendChain&&)      = default;
  DependentSendChain& operator= (const DependentSendChain&) = delete;
  DependentSendChain& operator= (DependentSendChain&&)      = default;

  ~DependentSendChain() = default;

 private:
  void checkInit() {
    // Done here rather than the constructor to make static
    // initialization of an instance safe
    if (last_epoch_ == no_epoch) {
      reset();
    }
  }

  void reset() {
    // Set up a 'closed' epoch so that we keep an invariant of always
    // having an epoch to call addAction on, rather than edge cases of
    // whether we've made one yet or not

    // The parameter `true` here tells VT to use an efficient rooted DS-epoch
    // by default. This can still be overridden by command-line flags
    last_epoch_ = theTerm()->makeEpochRooted(term::UseDS{true});
    theTerm()->finishedEpoch(last_epoch_);
  }

  EpochType last_epoch_ = no_epoch;
};

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_DEPENDENT_SEND_CHAIN_H*/
