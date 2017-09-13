
#if ! defined __RUNTIME_TRANSPORT_SEQ_ACTION__
#define __RUNTIME_TRANSPORT_SEQ_ACTION__

#include "seq_common.h"
#include "termination.h"

namespace vt { namespace seq {

template <typename MessageT>
struct Action {
  using ActionType = std::function<void(MessageT*)>;
  using CallableType = std::function<bool()>;

  SeqType const seq_id;
  ActionType const action;

  Action(SeqType const& in_seq_id, ActionType const& in_action)
    : seq_id(in_seq_id), action(in_action)
  { }

  void runAction(MessageT* msg) const {
    auto const callable = [this, msg]() -> bool {
      theTerm->consume();
      action(msg);
      return false;
    };

    contextualExecution(seq_id, false, callable);
  }

  CallableType generateCallable(MessageT* msg) const {
    return [msg,this](){
      runAction(msg);
      return false;
    };
  }
};

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_ACTION__*/
