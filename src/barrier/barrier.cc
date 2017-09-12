
#include "barrier.h"
#include "active.h"

namespace runtime { namespace barrier {

/*static*/ void
Barrier::barrier_up(BarrierMsg* msg) {
  the_barrier->barrier_up(
    msg->is_named, msg->is_wait, msg->barrier, msg->skip_term
  );
}

/*static*/ void
Barrier::barrier_down(BarrierMsg* msg) {
  the_barrier->barrier_down(msg->is_named, msg->is_wait, msg->barrier);
}

Barrier::barrier_state_t&
Barrier::insert_find_barrier(
  bool const& is_named, bool const& is_wait, BarrierType const& barrier,
  ActionType cont_action
) {
  auto& state = is_named ? named_barrier_state : unnamed_barrier_state;

  auto iter = state.find(barrier);
  if (iter == state.end()) {
    state.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(barrier),
      std::forward_as_tuple(barrier_state_t{is_named, barrier, is_wait})
    );
    iter = state.find(barrier);
  }

  if (cont_action != nullptr) {
    assert(
      (not is_wait) and
      "Barrier must not be waiting kind if action is associated"
    );

    iter->second.cont_action = cont_action;
  }

  return iter->second;
}

void
Barrier::remove_barrier(
  bool const& is_named, bool const& is_wait, BarrierType const& barrier
) {
  auto& state = is_named ? named_barrier_state : unnamed_barrier_state;

  auto iter = state.find(barrier);
  assert(
    iter != state.end() and "Barrier must exist at this point"
  );

  state.erase(iter);
}

BarrierType
Barrier::new_named_barrier() {
  setup_tree();
  BarrierType const next_barrier = cur_named_barrier++;
  return next_barrier;
}


void
Barrier::wait_barrier(BarrierType const& barrier, bool const skip_term) {
  setup_tree();

  bool const is_wait = true;
  bool const is_named = barrier != no_barrier;

  BarrierType const next_barrier = is_named ? barrier : cur_unnamed_barrier++;

  auto& barrier_state = insert_find_barrier(is_named, is_wait, next_barrier);

  barrier_up(is_named, is_wait, next_barrier, skip_term);

  while (not barrier_state.released) {
    the_msg->scheduler();
  }

  remove_barrier(is_named, is_wait, next_barrier);
}

void
Barrier::cont_barrier(
  ActionType fn, BarrierType const& barrier, bool const skip_term
) {
  setup_tree();

  bool const is_wait = false;
  bool const is_named = barrier != no_barrier;

  BarrierType const next_barrier = is_named ? barrier : cur_unnamed_barrier++;

  auto& barrier_state = insert_find_barrier(is_named, is_wait, next_barrier, fn);

  barrier_up(is_named, is_wait, next_barrier, skip_term);
}

void
Barrier::barrier_down(
  bool const& is_named, bool const& is_wait, BarrierType const& barrier
) {
  auto& barrier_state = insert_find_barrier(is_named, is_wait, barrier);

  barrier_state.released = true;

  if (not is_wait and barrier_state.cont_action != nullptr) {
    barrier_state.cont_action();
  }
}

void
Barrier::barrier_up(
  bool const& is_named, bool const& is_wait, BarrierType const& barrier,
  bool const& skip_term
) {
  setup_tree();

  auto& barrier_state = insert_find_barrier(is_named, is_wait, barrier);

  barrier_state.recv_event_count += 1;

  bool const is_ready = barrier_state.recv_event_count == num_children + 1;

  if (is_ready) {
    if (not is_root) {
      auto msg = new BarrierMsg(is_named, barrier, is_wait);
      // system-level barriers can choose to skip the termination protocol
      if (skip_term) {
        the_msg->set_term_message(msg);
      }
      the_msg->send_msg<BarrierMsg, barrier_up>(parent, msg, [=]{
        delete msg;
      });
    } else {
      auto msg = new BarrierMsg(is_named, barrier, is_wait);
      // system-level barriers can choose to skip the termination protocol
      if (skip_term) {
        the_msg->set_term_message(msg);
      }
      the_msg->broadcast_msg<BarrierMsg, barrier_down>(msg, [=]{
        delete msg;
      });
      barrier_down(is_named, is_wait, barrier);
    }
  }
}

}} //end namespace runtime::barrier
