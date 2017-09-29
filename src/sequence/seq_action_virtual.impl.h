
#if ! defined __RUNTIME_TRANSPORT_SEQ_ACTION_VIRTUAL_IMPL__
#define __RUNTIME_TRANSPORT_SEQ_ACTION_VIRTUAL_IMPL__

#include "config.h"
#include "seq_common.h"
#include "seq_action.h"

namespace vt { namespace seq {

template <typename MessageT, typename VcT>
ActionVirtual<MessageT, VcT>::ActionVirtual(
  SeqType const& in_seq_id, ActionType const& in_action
) : seq_id(in_seq_id), action(in_action)
{ }

template <typename MessageT, typename VcT>
void ActionVirtual<MessageT, VcT>::runAction(
  VcT* vc, MessageT* msg, bool consume
) const {
  auto const callable = [this, consume, msg, vc]() -> bool {
    if (consume) {
      theTerm->consume();
    }
    action(msg, vc);
    return false;
  };

  contextualExecutionVirtual(seq_id, false, callable);
}

template <typename MessageT, typename VcT>
typename ActionVirtual<MessageT, VcT>::CallableType
ActionVirtual<MessageT, VcT>::generateCallable(MessageT* msg, VcT* vc) const {
  return [msg,vc,this](){
    runAction(vc, msg);
    return false;
  };
}

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_ACTION_VIRTUAL_IMPL__*/
