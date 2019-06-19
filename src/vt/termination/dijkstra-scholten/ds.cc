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
TermDS<CommType>::TermDS(EpochType in_epoch, bool isRoot_, NodeType self_)
  : EpochRelation(in_epoch, true),
    parent(-1), self(self_), C(0), ackedArbitrary(0), ackedParent(0),
    reqedParent(0), engagementMessageCount(0), D(0), processedSum(C)
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
    outstanding.push_back(AckRequest(self, 0));
  }
}

template <typename CommType>
void TermDS<CommType>::msgSent(NodeType successor, CountType count) {
  vtAssertExpr(successor >= 0);
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

  debug_print(
    termds, node,
    "msgSent: epoch={:x}, to={}, count={}, C={}, D={}, lC={}, lD={}\n",
    epoch_, successor, count, C, D, lC, lD
  );
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
    "gotAck: epoch={:x}, count={}, parent={}, C={}, D={}, lC={}, lD={}\n",
    epoch_, count, parent, C, D, lC, lD
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

  vtAssertExpr(predecessor >= 0);

  bool const self_pred = predecessor == self;

  debug_print_verbose(
    termds, node,
    "msgProcessed: (pre) epoch={:x}, from={}, count={}, "
    "parent={}, outstanding.size()={}, C={}, D={}, lC={}, lD={}\n",
    epoch_, predecessor, count, parent, outstanding.size(),
    C, D, lC, lD
  );

  // Test for the self-process case that delays termination with local credit
  if (self_pred) {
    lC += count;
    vtAssertExprInfo(lC <= lD, lC, lD, epoch_, predecessor, count, C, D, parent);
    // May not be engaged if msgProcessed is not called before a local prod/cons
    // occurs
  } else {
    C += count;
    processedSum += count;
  }

  debug_print(
    termds, node,
    "msgProcessed: epoch={:x}, from={}, count={}, "
    "parent={}, outstanding.size()={}, C={}, D={}, lC={}, lD={}\n",
    epoch_, predecessor, count, parent, outstanding.size(),
    C, D, lC, lD
  );

  if (outstanding.size() == 0 and not self_pred) {
    debug_print(
      termds, node,
      "msgProcessed: engagement with new parent={}, epoch={:x}, count={}, "
      "C={}, D={}, lC={}, lD={}\n",
      predecessor, epoch_, count, C, D, lC, lD
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

  debug_print(
    termds, node,
    "tryAck: epoch={:x}, parent={}, emc={}, reqedParent={}, "
    "ackedParent={}, outstanding.size()={}, C={}, D={}, lC={}, lD={}\n",
    epoch_, parent, engagementMessageCount, reqedParent, ackedParent,
    outstanding.size(), C, D, lC, lD
  );

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
    "tryLast: epoch={:x}, parent={}, emc={}, reqedParent={}, "
    "ackedParent={}, outstanding.size()={}, C={}, D={}, lC={}, lD={}\n",
    epoch_, parent, engagementMessageCount, reqedParent, ackedParent,
    outstanding.size(), C, D, lC, lD
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
