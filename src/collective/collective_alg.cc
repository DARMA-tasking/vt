
#include "collective_alg.h"
#include "messaging/active.h"

namespace vt { namespace collective {

CollectiveAlg::CollectiveAlg()  : Tree(tree_cons_tag_t){ }

/*static*/ void CollectiveAlg::barrierUp(BarrierMsg* msg) {
  theCollective()->barrierUp(
      msg->is_named, msg->is_wait, msg->barrier, msg->skip_term
  );
}

/*static*/ void CollectiveAlg::barrierDown(BarrierMsg* msg) {
  theCollective()->barrierDown(msg->is_named, msg->is_wait, msg->barrier);
}

CollectiveAlg::BarrierStateType& CollectiveAlg::insertFindBarrier(
    bool const& is_named, bool const& is_wait, CollectiveAlgType const& barrier,
    ActionType cont_action
) {
  auto& state = is_named ? named_barrier_state_ : unnamed_barrier_state_;

  auto iter = state.find(barrier);
  if (iter == state.end()) {
    state.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(barrier),
        std::forward_as_tuple(BarrierStateType{is_named, barrier, is_wait})
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

void CollectiveAlg::removeBarrier(
    bool const& is_named, bool const& is_wait, CollectiveAlgType const& barrier
) {
  auto& state = is_named ? named_barrier_state_ : unnamed_barrier_state_;

  auto iter = state.find(barrier);
  assert(
      iter != state.end() and "Barrier must exist at this point"
  );

  state.erase(iter);
}

CollectiveAlgType CollectiveAlg::newNamedBarrier() {
  setupTree();
  CollectiveAlgType const next_barrier = cur_named_barrier_++;
  NodeType const cur_node = theContext()->getNode();
  CollectiveAlgType const cur_node_shift = static_cast<CollectiveAlgType>(cur_node) << 32;
  CollectiveAlgType const barrier_name = next_barrier | cur_node_shift;
  return barrier_name;
}

void CollectiveAlg::waitBarrier(CollectiveAlgType const& barrier, bool const skip_term) {
  setupTree();

  bool const is_wait = true;
  bool const is_named = barrier != no_barrier;

  CollectiveAlgType const next_barrier = is_named ? barrier : cur_unnamed_barrier_++;

  auto& barrier_state = insertFindBarrier(is_named, is_wait, next_barrier);

  barrierUp(is_named, is_wait, next_barrier, skip_term);

  while (not barrier_state.released) {
    theMsg()->scheduler();
  }

  removeBarrier(is_named, is_wait, next_barrier);
}

void CollectiveAlg::contBarrier(
    ActionType fn, CollectiveAlgType const& barrier, bool const skip_term
) {
  setupTree();

  bool const is_wait = false;
  bool const is_named = barrier != no_barrier;

  CollectiveAlgType const next_barrier = is_named ? barrier : cur_unnamed_barrier_++;

  auto& barrier_state = insertFindBarrier(is_named, is_wait, next_barrier, fn);

  barrierUp(is_named, is_wait, next_barrier, skip_term);
}

void CollectiveAlg::barrierDown(
    bool const& is_named, bool const& is_wait, CollectiveAlgType const& barrier
) {
  auto& barrier_state = insertFindBarrier(is_named, is_wait, barrier);

  barrier_state.released = true;

  if (not is_wait and barrier_state.cont_action != nullptr) {
    barrier_state.cont_action();
  }
}

void CollectiveAlg::barrierUp(
    bool const& is_named, bool const& is_wait, CollectiveAlgType const& barrier,
    bool const& skip_term
) {
  auto const& num_children_ = getNumChildren();
  bool const& is_root_ = isRoot();
  auto const& parent_ = getParent();

  // ToDo: Why setup again? Setup should be once per processor
  setupTree();

  // ToDo: Why we call this function again? (if you come from contBarrier,
  // ToDo: this is already called)
  auto& barrier_state = insertFindBarrier(is_named, is_wait, barrier);

  barrier_state.recv_event_count += 1;

  bool const is_ready = barrier_state.recv_event_count == num_children_ + 1;

  if (is_ready) {
    if (not is_root_) {
      auto msg = new BarrierMsg(is_named, barrier, is_wait);
      // system-level barriers can choose to skip the termination protocol
      if (skip_term) {
        theMsg()->setTermMessage(msg);
      }
      debug_print(
          barrier, node,
          "barrierUp: barrier=%llu\n", barrier
      );
      theMsg()->sendMsg<BarrierMsg, barrierUp>(parent_, msg, [=]{
        delete msg;
      });
    } else {
      auto msg = new BarrierMsg(is_named, barrier, is_wait);
      // system-level barriers can choose to skip the termination protocol
      if (skip_term) {
        theMsg()->setTermMessage(msg);
      }
      debug_print(
          barrier, node,
          "barrierDown: barrier=%llu\n", barrier
      );
      theMsg()->broadcastMsg<BarrierMsg, barrierDown>(msg, [=]{
        delete msg;
      });
      barrierDown(is_named, is_wait, barrier);
    }
  }
}


}}  // end namespace vt::collective