
#if !defined INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_DS_CC
#define INCLUDED_TERMINATION_DIJKSTRA_SCHOLTEN_DS_CC

#include "config.h"
#include "termination/dijkstra-scholten/ds.h"
#include "termination/dijkstra-scholten/comm.h"
#include "termination/dijkstra-scholten/ack_request.h"

namespace vt { namespace term { namespace ds {

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
  CommType::rootTerminated(epoch_);
}

template <typename CommType>
void TermDS<CommType>::setRoot(bool isRoot) {
  if (isRoot) {
    outstanding.push_back(AckRequest(NodeType(), 0));
  }
}

template <typename CommType>
void TermDS<CommType>::msgSent(NodeType successor) {
  debug_print(
    termds, node,
    "{} sent message to {}\n", self, successor
  );
  vtAssertInfo(
    1 && (C == processedSum - (ackedArbitrary + ackedParent)),
    "DS-invariant", C, D, processedSum, ackedArbitrary,
    ackedParent, reqedParent, outstanding.size(), engagementMessageCount,
    parent
  );
  D++;
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
void TermDS<CommType>::msgProcessed(NodeType const predecessor) {
  vtAssertInfo(
    4 && (C == processedSum - (ackedArbitrary + ackedParent)),
    "DS-invariant", C, D, processedSum, ackedArbitrary,
    ackedParent, reqedParent, outstanding.size(), engagementMessageCount,
    parent
  );

  C++;
  processedSum++;

  if (outstanding.size() == 0) {
    debug_print(
      termds, node,
      "{}: got engagement message from new parent={}, count={}, D={}\n",
      self, predecessor, 1, D
    );

    parent = predecessor;
    engagementMessageCount = 1;
    outstanding.push_front(AckRequest(predecessor, 1));
  } else {
    typename AckReqListType::iterator iter = outstanding.begin();
    ++iter;
    for (; iter != outstanding.end(); ++iter) {
      if (iter->pred == predecessor) {
        iter->count++;
        break;
      }
    }
    if (iter == outstanding.end()) {
      outstanding.push_back(AckRequest(predecessor, 1));
    }
  }

  if (predecessor == parent) {
    reqedParent += 1;
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
    "{}: tryLast: has parent={}, D={}, C={}, emc={}, reqedParent={}, "
    "ackedParent={}\n",
    self, parent, D, C, engagementMessageCount, reqedParent, ackedParent
  );

  if (outstanding.size() != 1) {
    return;
  }

  auto const engageEq = reqedParent - ackedParent == engagementMessageCount;
  if (engageEq && D == 0 && C == engagementMessageCount) {
    debug_print(
      termds, node,
      "{}: successful tryLast: parent={}, emc={}\n",
      self, parent, engagementMessageCount
    );

    AckRequest a = outstanding.back();
    outstanding.pop_back();
    vtAssertInfo(
      engagementMessageCount == a.count,
      "DS-invariant", C, D, processedSum, ackedArbitrary,
      ackedParent, reqedParent, outstanding.size(), engagementMessageCount,
      parent
    );

    if (a.pred == self) {
      CommType::rootTerminated(epoch_);
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
