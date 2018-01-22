
#if !defined INCLUDED_COLLECTIVE_COLLECTIVE_ALG_H
#define INCLUDED_COLLECTIVE_COLLECTIVE_ALG_H

#include <unordered_map>

#include "config.h"
#include "tree/tree.h"
#include "activefn/activefn.h"
#include "messaging/message.h"
#include "collective_alg_barrier_msg.h"
#include "collective_alg_barrier_state.h"

namespace vt { namespace collective {

constexpr CollectiveAlgType const fst_collective_alg = 1;

struct CollectiveAlg : Tree {
/*----------------------------------------------------------------------------
 *
 *  CollectiveAlg class implements all collective operations:
 *    1) Barrier
 *    2) One to all: broadcast, scatter
 *    3) All to one: reduce, gather
 *    4) All to all: allreduce, allgather, alltoall, reduce_scatter
 *    5) Scan etc.
 *
 *------------------------------------------------------------------------------
 */

  CollectiveAlg();

  template <typename T>
  using ContainerType = std::unordered_map<CollectiveAlgType, T>;

  /*----------------------------------------------------------------------------
   *            Begin Barrier
   *----------------------------------------------------------------------------
   *
   * Implements basic barrier functionality
   *
   *----------------------------------------------------------------------------
   */

  using BarrierStateType = BarrierState;

  BarrierStateType& insertFindBarrier(
      bool const& is_named, bool const& is_wait, CollectiveAlgType const& barrier,
      ActionType cont_action = nullptr
  );

  void removeBarrier(
      bool const& is_named, bool const& is_wait, CollectiveAlgType const& barrier
  );

  BarrierType newNamedBarrier();

  void barrierUp(
      bool const& is_named, bool const& is_wait, CollectiveAlgType const& barrier,
      bool const& skip_term
  );

  void barrierDown(
      bool const& is_named, bool const& is_wait, CollectiveAlgType const& barrier
  );

  inline void barrier(CollectiveAlgType const& barrier = no_barrier) {
    return waitBarrier(barrier);
  }

  inline void barrierThen(ActionType fn) {
    return contBarrier(fn);
  }

  inline void barrierThen(CollectiveAlgType const& barrier, ActionType fn) {
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


  /*
   *----------------------------------------------------------------------------
   *           End Barrier
   *----------------------------------------------------------------------------
   */

 private:
  void waitBarrier(
      CollectiveAlgType const& barrier = no_barrier, bool const skip_term = false
  );

  void contBarrier(
      ActionType fn, CollectiveAlgType const& barrier = no_barrier,
      bool const skip_term = false
  );

 private:
  CollectiveAlgType cur_named_barrier_ = fst_collective_alg;
  CollectiveAlgType cur_unnamed_barrier_ = fst_collective_alg;

  ContainerType<BarrierStateType> named_barrier_state_, unnamed_barrier_state_;

};

}}  // end namespace vt::collective

namespace vt {

extern collective::CollectiveAlg* theCollective();

} //end namespace vt


#endif /*INCLUDED_COLLECTIVE_COLLECTIVE_ALG_H*/
