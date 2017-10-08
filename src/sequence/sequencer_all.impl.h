#if ! defined __RUNTIME_TRANSPORT_SEQUENCE_ALL_IMPL__
#define __RUNTIME_TRANSPORT_SEQUENCE_ALL_IMPL__

#include "config.h"
#include "seq_common.h"
#include "sequencer.h"
#include "sequencer_virtual.h"

namespace vt { namespace seq {

template <typename Fn>
bool executeSeqExpandContext(SeqType const& id, SeqNodePtrType node, Fn&& fn) {
  if (theSeq->seq_manager->isVirtual(id)) {
    return theVirtualSeq->executeInNodeContext(id, node, fn);
  } else {
    return theSeq->executeInNodeContext(id, node, fn);
  }
}

inline void enqueueAction(SeqType const& id, ActionType const& action) {
  if (theSeq->seq_manager->isVirtual(id)) {
    return theVirtualSeq->enqueue(action);
  } else {
    return theSeq->enqueue(action);
  }
}

template <typename SeqTag, template <typename> class SeqTrigger>
/*static*/
typename TaggedSequencer<SeqTag, SeqTrigger>::SeqFunType
TaggedSequencer<SeqTag, SeqTrigger>::convertSeqFun(
  SeqType const& id, UserSeqFunType fn
)  {
  return [=]() -> bool {
    if (theSeq->seq_manager->isVirtual(id)) {
      return theVirtualSeq->lookupContextExecute(id, fn);
    } else {
      return theSeq->lookupContextExecute(id, fn);
    }
  };
}

}} //end namespace vt::seq

#endif /*__RUNTIME_TRANSPORT_SEQUENCE_ALL_IMPL_*/
