
#include "config.h"
#include "seq_common.h"
#include "seq_parallel.h"
#include "sequencer.h"
#include "term_headers.h"

namespace vt { namespace seq {

void SeqParallel::setTriggeredAction(ActionType action) {
  triggered_action_ = action;
}

SeqParallel::SeqFuncLen SeqParallel::getNumFuncs() const {
  return num_funcs_;
}

SeqParallel::SeqFuncLen SeqParallel::getNumFuncsCompleted() const {
  return num_funcs_completed_;
}

SeqParallel::SeqFuncLen SeqParallel::getSize() const {
  return num_funcs_ - num_funcs_completed_;
}

SeqNodeStateEnumType SeqParallel::expandParallelNode(SeqNodePtrType this_node) {
  debug_print(
    sequence, node,
    "SeqParallel: expandParallelNode: funcs=%u, skip_queue=%s, node=%p\n",
    num_funcs_, print_bool(seq_skip_queue), PRINT_SEQ_NODE_PTR(this_node)
  );

  auto cur_state = SeqNodeStateEnumType::KeepExpandingState;

  // Inform the termination detector to produce children nodes---ensures that
  // deferred execution of these does not cause termination be reached
  // incorrectly if the wait execution is delayed
  theTerm->produce(term::any_epoch_sentinel, par_funcs_.size());

  for (auto&& par_fn : par_funcs_) {
    debug_print(
      sequence, node,
      "SeqParallel: expandParallelNode: num_funcs_=%u, expanding\n", num_funcs_
    );

    if (seq_skip_queue) {
      bool const blocked = executeSeqExpandContext(seq_id_, this_node, par_fn);
      if (blocked) {
        cur_state = SeqNodeStateEnumType::WaitingNextState;
      }
    } else {
      auto defer_work = [=]{
        debug_print(
          sequence, node,
          "SeqParallel: parallel node: expand deferred: id=%d\n", seq_id_
        );

        executeSeqExpandContext(seq_id_, this_node, par_fn);
      };
      enqueueAction(seq_id_, defer_work);
    }
  }

  if (seq_skip_queue) {
    return cur_state;
  } else {
    return SeqNodeStateEnumType::WaitingNextState;
  }
}

bool SeqParallel::join() {
  auto const& old_val = num_funcs_completed_.fetch_add(1);

  debug_print(
    sequence, node,
    "SeqParallel: join: old_val=%d, num_funcs=%d\n", old_val, num_funcs_
  );

  // Inform the termination detector that a child is consumed
  theTerm->consume(term::any_epoch_sentinel, 1);

  if (old_val == num_funcs_ - 1) {
    if (triggered_action_ != nullptr) {
      triggered_action_();
    }
    return true;
  } else if (old_val > num_funcs_ - 1) {
    return true;
  } else {
    return false;
  }
}

}} //end namespace vt::seq
