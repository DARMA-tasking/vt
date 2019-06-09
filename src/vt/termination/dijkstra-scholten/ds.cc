/*
//@HEADER
// ************************************************************************
//
//                          ds.cc
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

#if !defined INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_DS_CC
#define INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_DS_CC

#include "vt/config.h"
#include "vt/termination/dijkstra-scholten/ds.h"
#include "vt/termination/dijkstra-scholten/comm.h"
#include "vt/termination/dijkstra-scholten/ack_request.h"
#include "vt/termination/termination.h"

namespace vt { namespace term { namespace ds {

template <typename CommType>
void TermDS<CommType>::addChildEpoch(EpochType const& epoch) {
  debug_print(
    termds, node,
    "addChildEpoch: epoch={:x}\n", epoch
  );

  // Produce a single work unit for the child epoch so it can not finish while
  // this epoch is live
  theTerm()->produce(epoch);
  children_.push_back(epoch);
}

template <typename CommType>
void TermDS<CommType>::clearChildren() {
  debug_print(
    termds, node,
    "clearChildren: epoch={:x}\n", epoch_
  );

  for (auto&& cur_epoch : children_) {
    theTerm()->consume(cur_epoch);
  }
  children_.clear();
}

template <typename CommType>
TermDS<CommType>::TermDS(EpochType in_epoch, bool isRoot_, NodeType self_)
  : parent(-1), self(self_), C(0), ackedArbitrary(0), ackedParent(0),
    reqedParent(0), engagementMessageCount(0), D(0), processedSum(C),
    epoch_(in_epoch)
{
  setRoot(isRoot_);
}

template <typename CommType>
void TermDS<CommType>::terminated() {
  debug_print(
    termds, node,
    "terminated: epoch={:x}\n", epoch_
  );

  CommType::rootTerminated(epoch_);
}

template <typename CommType>
void TermDS<CommType>::setRoot(bool isRoot) {
  if (isRoot) {
    outstanding.push_back(AckRequest(NodeType(), 0));
  }
}

template <typename CommType>
void TermDS<CommType>::msgSent(NodeType successor, CountType count) {
  debug_print(
    termds, node,
    "msgSent: from={} sent count={} message to {}\n", self, count, successor
  );
  vtAssertInfo(
    1 && (C == processedSum - (ackedArbitrary + ackedParent)),
    "DS-invariant", C, D, processedSum, ackedArbitrary,
    ackedParent, reqedParent, outstanding.size(), engagementMessageCount,
    parent
  );
  // Test for the self-send case that delays termination with local debt
  if (successor == self) {
    lD += count;
  } else {
    D += count;
  }
}

template <typename CommType>
void TermDS<CommType>::gotAck(CountType count) {
  vtAssertInfo(
    2 && (C == processedSum - (ackedArbitrary + ackedParent)),
    "DS-invariant", C, D, processedSum, ackedArbitrary,
    ackedParent, reqedParent, outstanding.size(), engagementMessageCount,
    parent
  );
  D -= count;
  debug_print(
    termds, node,
    "gotAck count={}, D={}\n", count, D
  );
  tryLast();
}

template <typename CommType>
void TermDS<CommType>::doneSending() {
  vtAssertInfo(
    3 && (C == processedSum - (ackedArbitrary + ackedParent)),
    "DS-invariant", C, D, processedSum, ackedArbitrary,
    ackedParent, reqedParent, outstanding.size(), engagementMessageCount,
    parent
  );
}

template <typename CommType>
void TermDS<CommType>::msgProcessed(NodeType predecessor, CountType count) {
  vtAssertInfo(
    4 && (C == processedSum - (ackedArbitrary + ackedParent)),
    "DS-invariant", C, D, processedSum, ackedArbitrary,
    ackedParent, reqedParent, outstanding.size(), engagementMessageCount,
    parent
  );

  bool const self_pred = predecessor == self;

  // Test for the self-process case that delays termination with local credit
  if (self_pred) {
    lC += count;
    vtAssertExpr(lC <= lD);
    // Must be engaged for a local credit/processing to increase
    vtAssertExpr(not (outstanding.size() == 0));
  } else {
    C += count;
    processedSum += count;
  }

  if (outstanding.size() == 0) {
    debug_print(
      termds, node,
      "got engagement message from new parent={}, count={}, D={}\n",
      predecessor, count, D
    );

    parent = predecessor;
    engagementMessageCount = count;
    outstanding.push_front(AckRequest(predecessor, count));
  } else if (not self_pred) {
    typename AckReqListType::iterator iter = outstanding.begin();
    ++iter;
    for (; iter != outstanding.end(); ++iter) {
      if (iter->pred == predecessor) {
        iter->count += count;
        break;
      }
    }
    if (iter == outstanding.end()) {
      outstanding.push_back(AckRequest(predecessor, count));
    }
  }

  if (predecessor == parent) {
    reqedParent += count;
  }

  tryAck();
  tryLast();
}

template <typename CommType>
void TermDS<CommType>::needAck(
  NodeType const predecessor, CountType const count
) {
  vtAssertInfo(
    5 && (C == processedSum - (ackedArbitrary + ackedParent)),
    "DS-invariant", C, D, processedSum, ackedArbitrary,
    ackedParent, reqedParent, outstanding.size(), engagementMessageCount,
    parent
  );
}

template <typename CommType>
void TermDS<CommType>::tryAck() {
  if (outstanding.size() <= 1) {
    return;
  }

  AckRequest a = outstanding.back();
  if (C >= a.count) {
    C -= a.count;
    if (hasParent() && a.pred == parent) {
      ackedParent += a.count;
    } else {
      ackedArbitrary += a.count;
    }
    outstanding.pop_back();
    CommType::acknowledge(epoch_, a.pred, a.count);
  }
}

template <typename CommType>
bool TermDS<CommType>::hasParent() {
  return !outstanding.empty();
}

template <typename CommType>
void TermDS<CommType>::tryLast() {
  debug_print(
    termds, node,
    "tryLast: parent={}, D={}, C={}, emc={}, reqedParent={}, "
    "ackedParent={}, outstanding.size()={}, lC={}, lD={}\n",
    parent, D, C, engagementMessageCount, reqedParent, ackedParent,
    outstanding.size(), lC, lD
  );

  if (outstanding.size() != 1 or lC not_eq lD) {
    return;
  }

  auto const engageEq = reqedParent - ackedParent == engagementMessageCount;

  debug_print(
    termds, node,
    "tryLast: parent={}, D={}, C={}, emc={}, reqedParent={}, "
    "ackedParent={}, engageEq={}\n",
    parent, D, C, engagementMessageCount, reqedParent, ackedParent,
    engageEq
  );

  if (engageEq && D == 0 && C == engagementMessageCount) {
    AckRequest a = outstanding.back();
    outstanding.pop_back();

    vtAssertInfo(
      engagementMessageCount == a.count,
      "DS-invariant", C, D, processedSum, ackedArbitrary,
      ackedParent, reqedParent, outstanding.size(), engagementMessageCount,
      parent
    );

    debug_print(
      termds, node,
      "successful tryLast: parent={}, emc={}, a.pred={}\n",
      parent, engagementMessageCount, a.pred
    );

    if (a.pred == self) {
      terminated();
    } else {
      vtAssertInfo(
        parent == a.pred,
        "DS-invariant", C, D, processedSum, ackedArbitrary,
        ackedParent, reqedParent, outstanding.size(), engagementMessageCount,
        parent
      );
      CommType::acknowledge(epoch_, parent, engagementMessageCount);
    }

    parent = -1;
    C = ackedParent = ackedArbitrary = reqedParent = 0;
    engagementMessageCount = processedSum = 0;
  }
}

template struct TermDS<StateDS>;

}}} /* end namespace vt::term::ds */

#endif /*INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_DS_CC*/
