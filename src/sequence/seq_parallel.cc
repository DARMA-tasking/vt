
#include "config.h"
#include "seq_common.h"
#include "seq_parallel.h"
#include "sequencer.h"

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
        executeSeqExpandContext(seq_id_, this_node, par_fn);
      };
      enqueue_action(defer_work);
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
