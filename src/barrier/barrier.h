
#if ! defined __RUNTIME_TRANSPORT_BARRIER__
#define __RUNTIME_TRANSPORT_BARRIER__

#include <unordered_map>

#include "common.h"
#include "function.h"
#include "message.h"
#include "tree.h"
#include "barrier_msg.h"

namespace vt { namespace barrier {

// struct ProcessGroup { };
// struct GroupBarrier { };

constexpr BarrierType const fst_barrier = 1;

struct Barrier : Tree {
  using BarrierStateType = BarrierState;

  template <typename T>
  using ContainerType = std::unordered_map<BarrierType, T>;

  BarrierStateType& insert_find_barrier(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier,
    ActionType cont_action = nullptr
  );

  void remove_barrier(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier
  );

  BarrierType new_named_barrier();

  void barrier_up(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier,
    bool const& skip_term
  );

  void barrier_down(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier
  );

  inline void barrier(BarrierType const& barrier = no_barrier) {
    return wait_barrier(barrier);
  }

  inline void barrier_then(ActionType fn) {
    return cont_barrier(fn);
  }

  inline void barrier_then(BarrierType const& barrier, ActionType fn) {
    return cont_barrier(fn, barrier);
  }

  inline void system_meta_barrier() {
    bool const skip_term = true;
    return wait_barrier(no_barrier, skip_term);
  }

  inline void system_meta_barrier_cont(ActionType fn) {
    bool const skip_term = true;
    return cont_barrier(fn, no_barrier, skip_term);
  }

  static void barrier_up(BarrierMsg* msg);
  static void barrier_down(BarrierMsg* msg);

private:

  void wait_barrier(
    BarrierType const& barrier = no_barrier, bool const skip_term = false
  );

  void cont_barrier(
    ActionType fn, BarrierType const& barrier = no_barrier,
    bool const skip_term = false
  );

private:
  BarrierType cur_named_barrier_ = fst_barrier;
  BarrierType cur_unnamed_barrier_ = fst_barrier;

  ContainerType<BarrierStateType> named_barrier_state_, unnamed_barrier_state_;
};

}} //end namespace vt::barrier

namespace vt {

extern std::unique_ptr<barrier::Barrier> theBarrier;

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_BARRIER__*/
