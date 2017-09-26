
#if ! defined __RUNTIME_TRANSPORT_BARRIER__
#define __RUNTIME_TRANSPORT_BARRIER__

#include <unordered_map>

#include "config.h"
#include "registry_function.h"
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

  BarrierStateType& insertFindBarrier(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier,
    ActionType cont_action = nullptr
  );

  void removeBarrier(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier
  );

  BarrierType newNamedBarrier();

  void barrierUp(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier,
    bool const& skip_term
  );

  void barrierDown(
    bool const& is_named, bool const& is_wait, BarrierType const& barrier
  );

  inline void barrier(BarrierType const& barrier = no_barrier) {
    return waitBarrier(barrier);
  }

  inline void barrierThen(ActionType fn) {
    return contBarrier(fn);
  }

  inline void barrierThen(BarrierType const& barrier, ActionType fn) {
    return contBarrier(fn, barrier);
  }

  inline void systemMetaBarrier() {
    bool const skip_term = true;
    return waitBarrier(no_barrier, skip_term);
  }

  inline void systemMetaBarrierCont(ActionType fn) {
    bool const skip_term = true;
    return contBarrier(fn, no_barrier, skip_term);
  }

  static void barrierUp(BarrierMsg* msg);
  static void barrierDown(BarrierMsg* msg);

private:

  void waitBarrier(
    BarrierType const& barrier = no_barrier, bool const skip_term = false
  );

  void contBarrier(
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
