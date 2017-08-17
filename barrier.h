
#if ! defined __RUNTIME_TRANSPORT_BARRIER__
#define __RUNTIME_TRANSPORT_BARRIER__

#include <unordered_map>

#include "common.h"
#include "function.h"
#include "message.h"
#include "tree.h"

namespace runtime { namespace barrier {

// struct ProcessGroup { };
// struct GroupBarrier { };

constexpr barrier_t const fst_barrier = 1;

struct BarrierMsg : runtime::ShortMessage {
  bool is_named, is_wait, skip_term = false;
  barrier_t barrier;

  BarrierMsg(
    bool const& in_is_named, barrier_t const& in_barrier, bool const& in_is_wait
  )
    : ShortMessage(), is_named(in_is_named), is_wait(in_is_wait),
      barrier(in_barrier), skip_term(false)
  { }
};

struct BarrierState {
  barrier_t barrier;

  int recv_event_count = 0;
  bool is_wait = false;
  bool is_named = false;
  bool released = false;

  action_t cont_action = nullptr;

  BarrierState(
    bool const& in_is_named, barrier_t const& in_barrier, bool const& in_is_wait
  ) : is_named(in_is_named), is_wait(in_is_wait), barrier(in_barrier)
  { }
};

struct Barrier : Tree {
  using barrier_state_t = BarrierState;

  template <typename T>
  using container_t = std::unordered_map<barrier_t, T>;

  barrier_state_t&
  insert_find_barrier(
    bool const& is_named, bool const& is_wait, barrier_t const& barrier,
    action_t cont_action = nullptr
  );

  void
  remove_barrier(
    bool const& is_named, bool const& is_wait, barrier_t const& barrier
  );

  barrier_t
  new_named_barrier();

  void
  barrier_up(
    bool const& is_named, bool const& is_wait, barrier_t const& barrier,
    bool const& skip_term
  );

  void
  barrier_down(
    bool const& is_named, bool const& is_wait, barrier_t const& barrier
  );

  inline void
  barrier(barrier_t const& barrier = no_barrier) {
    return wait_barrier(barrier);
  }

  inline void
  barrier_then(action_t fn) {
    return cont_barrier(fn);
  }

  inline void
  barrier_then(barrier_t const& barrier, action_t fn) {
    return cont_barrier(fn, barrier);
  }

  inline void
  system_meta_barrier() {
    bool const skip_term = true;
    return wait_barrier(no_barrier, skip_term);
  }

  inline void
  system_meta_barrier_cont(action_t fn) {
    bool const skip_term = true;
    return cont_barrier(fn, no_barrier, skip_term);
  }

  static void
  register_barrier_handlers();

  handler_t barrier_up_han = uninitialized_handler;
  handler_t barrier_down_han = uninitialized_handler;

private:

  void
  wait_barrier(
    barrier_t const& barrier = no_barrier, bool const skip_term = false
  );

  void
  cont_barrier(
    action_t fn, barrier_t const& barrier = no_barrier,
    bool const skip_term = false
  );

private:

  barrier_t cur_named_barrier = fst_barrier;
  barrier_t cur_unnamed_barrier = fst_barrier;

  container_t<barrier_state_t> named_barrier_state, unnamed_barrier_state;
};


}} //end namespace runtime::barrier

namespace runtime {

extern std::unique_ptr<barrier::Barrier> the_barrier;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_BARRIER__*/
