
#if !defined INCLUDED_SEQUENCE_SEQ_ACTION_VIRTUAL_H
#define INCLUDED_SEQUENCE_SEQ_ACTION_VIRTUAL_H

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/termination/termination.h"

namespace vt { namespace seq {

template <typename MessageT, typename VcT>
struct ActionVirtual {
  using ActionType = std::function<void(MessageT*, VcT*)>;
  using CallableType = std::function<bool()>;

  SeqType const seq_id;
  ActionType const action;

  ActionVirtual(SeqType const& in_seq_id, ActionType const& in_action);

  void runAction(VcT* vc, MessageT* msg, bool const consume = true) const;
  CallableType generateCallable(MessageT* msg, VcT* vc) const;
};

}} //end namespace vt::seq

#include "vt/sequence/seq_action_virtual.impl.h"

#endif /* INCLUDED_SEQUENCE_SEQ_ACTION_VIRTUAL_H*/
