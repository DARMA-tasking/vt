
#if ! defined __RUNTIME_TRANSPORT_SEQ_ACTION__
#define __RUNTIME_TRANSPORT_SEQ_ACTION__

#include "seq_common.h"
#include "termination.h"

namespace runtime { namespace seq {

template <typename MessageT>
struct Action {
  using action_t = std::function<void(MessageT*)>;
  using callable_t = std::function<bool()>;

  seq_t const seq_id;
  action_t const action;

  Action(seq_t const& in_seq_id, action_t const& in_action)
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

  callable_t generate_callable(MessageT* msg) const {
    return [msg,this](){
      run_action(msg);
      return false;
    };
  }
};

}} //end namespace runtime::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_ACTION__*/
