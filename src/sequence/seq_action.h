
#if !defined INCLUDED_SEQUENCE_SEQ_ACTION_H
#define INCLUDED_SEQUENCE_SEQ_ACTION_H

#include "config.h"
#include "seq_common.h"
#include "termination/termination.h"

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

#include "seq_action.impl.h"

#endif /* INCLUDED_SEQUENCE_SEQ_ACTION_H*/
