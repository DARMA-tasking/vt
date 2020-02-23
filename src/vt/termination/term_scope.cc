/*
//@HEADER
// *****************************************************************************
//
//                                term_scope.cc
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
#include "vt/termination/termination.h"
#include "vt/termination/term_common.h"
#include "vt/scheduler/scheduler.h"
#include "vt/messaging/active.h"
#include "vt/utils/bits/bits_common.h"

namespace vt { namespace term {

/*static*/ EpochType TerminationDetector::Scoped::rooted(
  bool small, ActionType closure
) {
  // For now we just use Dijkstra-Scholten if the region is "small"
  bool const use_dijkstra_scholten = small == true;
  auto const epoch = theTerm()->makeEpochRooted(use_dijkstra_scholten);
  bool term_finished = false;
  auto action = [&]{ term_finished = true; };
  theTerm()->addActionEpoch(epoch,action);
  vtAssertExpr(closure != nullptr);
  theMsg()->pushEpoch(epoch);
  closure();
  theMsg()->popEpoch();
  theTerm()->finishedEpoch(epoch);

  if (!term_finished) {
    auto sched = theSched()->beginNestedScheduling();
    while (!term_finished) {
      sched.runScheduler();
    }
  }

  return epoch;
}

/*static*/ EpochType TerminationDetector::Scoped::rooted(
  bool small, ActionType closure, ActionType action
) {
  bool const use_dijkstra_scholten = small == true;
  auto const epoch = theTerm()->makeEpochRooted(use_dijkstra_scholten);
  theTerm()->addActionEpoch(epoch,action);
  vtAssertExpr(closure != nullptr);
  theMsg()->pushEpoch(epoch);
  closure();
  theMsg()->popEpoch();
  theTerm()->finishedEpoch(epoch);
  return epoch;
}

/*static*/ EpochType TerminationDetector::Scoped::collective(
  ActionType closure
) {
  auto const epoch = theTerm()->makeEpochCollective(true);
  bool term_finished = false;
  auto action = [&]{ term_finished = true; };
  theTerm()->addActionEpoch(epoch,action);
  vtAssertExpr(closure != nullptr);
  theMsg()->pushEpoch(epoch);
  closure();
  theMsg()->popEpoch();
  theTerm()->finishedEpoch(epoch);

  if (!term_finished) {
    auto sched = theSched()->beginNestedScheduling();
    while (!term_finished) {
      sched.runScheduler();
    }
  }

  return epoch;
}

/*static*/ EpochType TerminationDetector::Scoped::collective(
  ActionType closure, ActionType action
) {
  auto const epoch = theTerm()->makeEpochCollective(true);
  theTerm()->addActionEpoch(epoch,action);
  vtAssertExpr(closure != nullptr);
  theMsg()->pushEpoch(epoch);
  closure();
  theMsg()->popEpoch();
  theTerm()->finishedEpoch(epoch);
  return epoch;
}

}} /* end namespace vt::term */
