
#if ! defined __RUNTIME_TRANSPORT_SEQ_ACTION_IMPL__
#define __RUNTIME_TRANSPORT_SEQ_ACTION_IMPL__

#include "config.h"
#include "seq_common.h"
#include "seq_action.h"

namespace vt { namespace seq {

template <typename MessageT>
Action<MessageT>::Action(SeqType const& in_seq_id, ActionType const& in_action)
  : seq_id(in_seq_id), action(in_action)
{ }

template <typename MessageT>
void Action<MessageT>::runAction(MessageT* msg, bool consume) const {
  auto const callable = [this, consume, msg]() -> bool {
    if (consume) {
      theTerm->consume();
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

#endif /* __RUNTIME_TRANSPORT_SEQ_ACTION_IMPL__*/
