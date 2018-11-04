
#if !defined INCLUDED_SEQUENCE_SEQ_ACTION_IMPL_H
#define INCLUDED_SEQUENCE_SEQ_ACTION_IMPL_H

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_action.h"

namespace vt { namespace seq {

template <typename MessageT>
Action<MessageT>::Action(SeqType const& in_seq_id, ActionType const& in_action)
  : seq_id(in_seq_id), action(in_action)
{ }

template <typename MessageT>
void Action<MessageT>::runAction(MessageT* msg, bool consume) const {
  auto const callable = [this, consume, msg]() -> bool {
    if (consume) {
      theTerm()->consume();
    }
    action(msg);
    return false;
  };

  contextualExecution(seq_id, false, callable);
}

template <typename MessageT>
typename Action<MessageT>::CallableType
Action<MessageT>::generateCallable(MessageT* msg) const {
  return [msg,this](){
    runAction(msg);
    return false;
  };
}

}} //end namespace vt::seq

#endif /* INCLUDED_SEQUENCE_SEQ_ACTION_IMPL_H*/
