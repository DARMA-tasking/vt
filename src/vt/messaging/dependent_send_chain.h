/*
//@HEADER
// ************************************************************************
//
//                          pending_send.h
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

#if !defined INCLUDED_VT_MESSAGING_DEPENDENT_SEND_CHAIN_H
#define INCLUDED_VT_MESSAGING_DEPENDENT_SEND_CHAIN_H

#include "vt/config.h"
#include "vt/messaging/pending_send.h"
#include "vt/epoch/epoch.h"

#include <functional>

namespace vt { namespace messaging {

class DependentSendChain final {
 public:
  DependentSendChain() { }

  // Add a task to the chain of work, with subsequent tasks dependent
  // on just that task
  void add(PendingSend&& link) {
    // Note that last_epoch_ may be uninitialized until the
    // checkInit() call inside add(EpochType, PendingSend)

    EpochType new_epoch = theTerm()->makeEpochRooted();
    add(new_epoch, std::move(link));
  }

  // Add a task to the chain of work, with subsequent tasks dependent
  // on all work occuring in the specified epoch
  void add(EpochType new_epoch, PendingSend&& link) {
    checkInit();

    theTerm()->addAction(last_epoch_, [new_epoch, ps = std::move(link)] () mutable {
        theMsg()->pushEpoch(new_epoch);
        ps.release();
        theMsg()->popEpoch(new_epoch);
        theTerm()->finishedEpoch(new_epoch);
        });
    last_epoch_ = new_epoch;
  }

  void done() {
    reset();
  }

  DependentSendChain            (const DependentSendChain&) = delete;
  DependentSendChain            (DependentSendChain&&)      = delete;
  DependentSendChain& operator= (const DependentSendChain&) = delete;
  DependentSendChain& operator= (DependentSendChain&&)      = delete;

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
    last_epoch_ = theTerm()->makeEpochRooted();
    theTerm()->finishedEpoch(last_epoch_);
  }

  EpochType last_epoch_ = no_epoch;
};

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_DEPENDENT_SEND_CHAIN_H*/
