
#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_H

#include "vt/config.h"
#include "vt/collective/reduce/reduce.fwd.h"
#include "vt/collective/reduce/reduce_hash.h"
#include "vt/collective/reduce/reduce_state.h"
#include "vt/collective/reduce/reduce_msg.h"
#include "vt/collective/reduce/operators/default_msg.h"
#include "vt/collective/reduce/operators/default_op.h"
#include "vt/messaging/active.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/message.h"
#include "vt/collective/tree/tree.h"
#include "vt/utils/hash/hash_tuple.h"

#include <tuple>
#include <unordered_map>
#include <cassert>
#include <cstdint>

namespace vt { namespace collective { namespace reduce {

struct Reduce : virtual collective::tree::Tree {
  using ReduceStateType = ReduceState;
  using ReduceNumType = ReduceState::ReduceNumType;

  Reduce();
  Reduce(GroupType const& group, collective::tree::Tree* in_tree);

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EpochType reduce(
    NodeType const& root, MessageT* const msg, TagType const& tag = no_tag,
    EpochType const& epoch = no_epoch, ReduceNumType const& num_contrib = 1,
    VirtualProxyType const& proxy = no_vrt_proxy
  );

  template <typename MessageT>
  void reduceAddMsg(
    MessageT* msg, bool const local, ReduceNumType const& num_contrib = -1
  );

  template <typename MessageT>
  void reduceNewMsg(MessageT* msg);

  /*
   *  Explicitly start the reduction when the number of contributions is not
   *  known up front
   */
  template <typename MessageT>
  void startReduce(
    TagType const& tag, EpochType const& epoch, VirtualProxyType const& proxy,
    bool use_num_contrib = true
  );

  template <typename MessageT>
  static void reduceRootRecv(MessageT* msg);
  template <typename MessageT>
  static void reduceUp(MessageT* msg);

private:
  std::unordered_map<ReduceEpochLookupType,EpochType> next_epoch_for_tag_;
  std::unordered_map<ReduceIdentifierType,ReduceStateType> live_reductions_;
  GroupType group_ = default_group;
};

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_H*/
