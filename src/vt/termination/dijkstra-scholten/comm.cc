/*
//@HEADER
// *****************************************************************************
//
//                                   comm.cc
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

#if !defined INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_COMM_CC
#define INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_COMM_CC

#include "vt/config.h"
#include "vt/termination/termination.h"
#include "vt/termination/dijkstra-scholten/comm.h"
#include "vt/termination/dijkstra-scholten/ds.h"
#include "vt/messaging/active.h"
#include "vt/context/context.h"
#include "vt/scheduler/scheduler.h"

namespace vt { namespace term { namespace ds {

/*static*/ void StateDS::requestAck(
  EpochType epoch, Endpoint successor, int64_t count
) {
  vt_debug_print(
    termds, node,
    "StateDS::requestAck: epoch={:x}, successor={}, count={}\n",
    epoch, successor, count
  );
  auto const node = theContext()->getNode();
  vtAssertExpr(successor != node);
  auto msg = makeMessage<AckMsg>(epoch,node,successor,count);
  theMsg()->markAsTermMessage(msg);
  theMsg()->sendMsg<AckMsg,requestAckHan>(successor, msg);
}

/*static*/ void StateDS::acknowledge(
  EpochType epoch, Endpoint predecessor, int64_t count
) {
  vt_debug_print(
    termds, node,
    "StateDS::acknowledge: epoch={:x}, predecessor={}, count={}\n",
    epoch, predecessor, count
  );
  auto const node = theContext()->getNode();
  vtAssertExpr(predecessor != node);
  auto msg = makeMessage<AckMsg>(epoch,node,predecessor,count);
  theMsg()->markAsTermMessage(msg);
  theMsg()->sendMsg<AckMsg,acknowledgeHan>(predecessor, msg);
}

/*static*/ void StateDS::rootTerminated(EpochType epoch) {
  vt_debug_print(
    termds, node,
    "StateDS::rootTerminated: epoch={:x}\n", epoch
  );
  theTerm()->epochTerminated(epoch, TerminationDetector::CallFromEnum::Root);
}

/*static*/ void StateDS::disengage(EpochType epoch) {
  vt_debug_print(
    termds, node,
    "StateDS::disengage: epoch={:x}\n", epoch
  );

  // We are likely inside code that holds a reference to this class for holding
  // DS terminator state. Thus, we can't remove it safely right here. So we will
  // enqueue an action to do the cleanup. This action must see if the DS term is
  // still disengaged (it can easily be re-engaged after it disengages).
  theSched()->enqueue([epoch]{
    auto ptr = theTerm()->getDSTerm(epoch);
    if (ptr != nullptr) {
      if (not ptr->isEngaged()) {
        theTerm()->cleanupEpoch(epoch, TerminationDetector::CallFromEnum::NonRoot);
      }
    }
  });
}

/*static*/ StateDS::TerminatorType*
StateDS::getTerminator(EpochType const& epoch) {
  auto term_iter = theTerm()->term_.find(epoch);
  if (term_iter == theTerm()->term_.end()) {
    auto const this_node = theContext()->getNode();
    theTerm()->term_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(epoch),
      std::forward_as_tuple(
        TerminatorType{epoch,false,this_node}
      )
    );
    term_iter = theTerm()->term_.find(epoch);
  }
  return &term_iter->second;
}

/*static*/ void StateDS::requestAckHan(AckMsg* msg) {
  auto const epoch = msg->getEpoch();
  auto term = getTerminator(epoch);
  term->needAck(msg->getNode(),msg->getCount());
}

/*static*/ void StateDS::acknowledgeHan(AckMsg* msg) {
  auto const epoch = msg->getEpoch();
  auto term = getTerminator(epoch);
  term->gotAck(msg->getCount());
}

/*static*/ void StateDS::rootTerminatedHan(AckMsg* msg) {
  auto const epoch = msg->getEpoch();
  auto term = getTerminator(epoch);
  term->terminated();
}

}}} /* end namespace vt::term::ds */

#endif /*INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_COMM_CC*/
