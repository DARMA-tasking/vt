/*
//@HEADER
// *****************************************************************************
//
//                           epoch_garbage_collect.cc
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
#include "vt/epoch/epoch_garbage_collect.h"
#include "vt/epoch/garbage_collect_msg.h"
#include "vt/epoch/epoch_manip.h"
#include "vt/collective/reduce/reduce_scope.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/pipe/pipe_manager.h"

namespace vt { namespace epoch {

void GarbageCollectTrait::reducedEpochs(GarbageCollectMsg* msg) {
  auto const ep = msg->getEpoch();
  auto iter = waiting_.find(ep);

  vtAssert(
    iter == waiting_.end(),
    "An epoch archetype can only garbage collect one set at a time"
  );

  // Save the message, waiting for confirmation reduction
  waiting_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(ep),
    std::forward_as_tuple(
      makeMessage<GarbageCollectMsg>(ep, msg->getVal().getSet())
    )
  );

  NodeType collective_root = 0;

  auto proxy = theEpoch()->getProxy();
  objgroup::proxy::Proxy<EpochManip> p{proxy};

  auto cb = theCB()->makeBcast<
    EpochManip, GarbageConfirmMsg, &EpochManip::confirmCollectEpochs
  >(p);
  auto msg_send = makeMessage<GarbageConfirmMsg>(ep);

  using collective::reduce::makeStamp;
  using collective::reduce::StrongEpoch;

  auto r = theEpoch()->reducer();

  // Stamp this with the epoch's archetype which ensures these don't get mixed
  // up across epoch types garbage collecting concurrently
  auto stamp = makeStamp<StrongEpoch>(ep);
  r->reduce<collective::None>(collective_root, msg_send.get(), cb, stamp);
}

void GarbageCollectTrait::confirmedReducedEpochs(GarbageConfirmMsg* msg) {
  auto const ep = msg->getEpoch();
  auto iter = waiting_.find(ep);

  vtAssert(
    iter != waiting_.end(),
    "Must have a valid waiting garbage collection message---just confirmed"
  );

  auto stored = iter->second;

  auto eiter = theEpoch()->terminated_epochs_.find(ep);
  vtAssert(
    eiter != theEpoch()->terminated_epochs_.end(),
    "Must have valid epoch type"
  );

  eiter->second->garbageCollect(stored->getVal().getSet());
  waiting_.erase(iter);
}


}} /* end namespace vt::epoch */
