
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

  void run_action(MessageT* msg) const {
    auto const callable = [this, msg]() -> bool {
      the_term->consume();
      action(msg);
      return false;
    };

    contextual_execution(seq_id, false, callable);
  }

  CallableType generate_callable(MessageT* msg) const {
    return [msg,this](){
      run_action(msg);
      return false;
    };
  }
};

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_ACTION__*/
