
#if !defined INCLUDED_SEQUENCE_SEQ_ACTION_H
#define INCLUDED_SEQUENCE_SEQ_ACTION_H

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/termination/termination.h"

namespace vt { namespace seq {

template <typename MessageT>
struct Action {
  using ActionType = std::function<void(MessageT*)>;
  using CallableType = std::function<bool()>;

  SeqType const seq_id;
  ActionType const action;

  Action(SeqType const& in_seq_id, ActionType const& in_action);

  void runAction(MessageT* msg, bool const consume = true) const;
  CallableType generateCallable(MessageT* msg) const;
};

}} //end namespace vt::seq

#include "vt/sequence/seq_action.impl.h"

#endif /* INCLUDED_SEQUENCE_SEQ_ACTION_H*/
