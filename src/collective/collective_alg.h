
#if !defined INCLUDED_COLLECTIVE_COLLECTIVE_ALG_H
#define INCLUDED_COLLECTIVE_COLLECTIVE_ALG_H

#include <unordered_map>

#include "config.h"
#include "tree/tree.h"
#include "activefn/activefn.h"
#include "messaging/message.h"
#include "collective_alg_barrier_msg.h"
#include "collective_alg_barrier_state.h"
#include "collective_alg_reduce_msg.h"
#include "collective_alg_reduce_state.h"
#include "utils/hash/hash_tuple.h"

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

  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------

  /*----------------------------------------------------------------------------
   *            Begin Barrier
   *----------------------------------------------------------------------------
   *
   * Implements basic barrier functionality
   *
   *----------------------------------------------------------------------------
   */

 public:
  inline void barrier(CollectiveAlgType const& barrier = no_barrier) {
    return waitBarrier(barrier);
  }

  inline void barrierThen(ActionType fn) {
    return contBarrier(fn);
  }

  BarrierType newNamedBarrier();

 private:
  using BarrierStateType = BarrierState;
  CollectiveAlgType cur_named_barrier_ = fst_collective_alg;
  CollectiveAlgType cur_unnamed_barrier_ = fst_collective_alg;

  ContainerType<BarrierStateType> named_barrier_state_, unnamed_barrier_state_;

  inline void barrierThen(CollectiveAlgType const& barrier, ActionType fn) {
    return contBarrier(fn, barrier);
  }

  void waitBarrier(
      CollectiveAlgType const& barrier = no_barrier,
      bool const skip_term = false
  );

  void contBarrier(
      ActionType fn, CollectiveAlgType const& barrier = no_barrier,
      bool const skip_term = false
  );

  inline void systemMetaBarrier() {
    bool const skip_term = true;
    return waitBarrier(no_barrier, skip_term);
  }

  inline void systemMetaBarrierCont(ActionType fn) {
    bool const skip_term = true;
    return contBarrier(fn, no_barrier, skip_term);
  }

  static void barrierUp(BarrierMsg *msg);
  static void barrierDown(BarrierMsg *msg);

  BarrierStateType& insertFindBarrier(
      bool const& is_named,
      bool const& is_wait,
      CollectiveAlgType const& barrier,
      ActionType cont_action = nullptr
  );

  void removeBarrier(
      bool const& is_named,
      bool const& is_wait,
      CollectiveAlgType const& barrier
  );

  void barrierUp(
      bool const& is_named,
      bool const& is_wait,
      CollectiveAlgType const& barrier,
      bool const& skip_term
  );

  void barrierDown(
      bool const& is_named,
      bool const& is_wait,
      CollectiveAlgType const& barrier
  );

  /*
   *----------------------------------------------------------------------------
   *           End Barrier
   *----------------------------------------------------------------------------
   */

  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------

  /*----------------------------------------------------------------------------
 *            Begin Reduce
 *----------------------------------------------------------------------------
 *
 * Implements basic reduction functionality
 *
 *----------------------------------------------------------------------------
 */

 public:
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  void reduce(
      NodeType const& root, MessageT* const msg, TagType const& tag = no_tag
  );

 private:
  using ReduceIdentifierType = std::tuple<TagType, EpochType>;
  using ReduceStateType = ReduceState;
  std::unordered_map<TagType, EpochType> next_epoch_for_tag_;
  std::unordered_map<ReduceIdentifierType, ReduceStateType> live_reductions_;

  template <typename MessageT>
  void reduceNewMsg(MessageT* msg);

  template <typename MessageT>
  static void reduceRootRecv(MessageT* msg);

  template <typename MessageT>
  static void reduceUp(MessageT* msg);

  /*
   *----------------------------------------------------------------------------
   *           End Reduce
   *----------------------------------------------------------------------------
   */

  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------

};

}}  // end namespace vt::collective

namespace vt {

extern collective::CollectiveAlg *theCollective();

} //end namespace vt

#include "collective_alg_reduce_impl.h"

#endif /*INCLUDED_COLLECTIVE_COLLECTIVE_ALG_H*/










































































