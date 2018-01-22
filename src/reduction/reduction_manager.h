
#if !defined INCLUDED_REDUCTION_REDUCTION_MANAGER_H
#define INCLUDED_REDUCTION_REDUCTION_MANAGER_H

#include "config.h"
#include "reduction/reduction_state.h"
#include "reduction/reduce_msg.h"
#include "messaging/active.h"
#include "activefn/activefn.h"
#include "messaging/message.h"
#include "tree/tree.h"
#include "utils/hash/hash_tuple.h"

#include <tuple>
#include <unordered_map>
#include <cassert>

namespace vt { namespace reduction {

struct ReductionManager : Tree {
  using ReduceIdentifierType = std::tuple<TagType, EpochType>;
  using ReduceStateType = State;

  ReductionManager();

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  void reduce(
    NodeType const& root, MessageT* const msg, TagType const& tag = no_tag
  );

  template <typename MessageT>
  void reduceNewMsg(MessageT* msg);

  template <typename MessageT>
  static void reduceRootRecv(MessageT* msg);
  template <typename MessageT>
  static void reduceUp(MessageT* msg);

private:
  std::unordered_map<TagType, EpochType> next_epoch_for_tag_;
  std::unordered_map<ReduceIdentifierType, ReduceStateType> live_reductions_;
};

} /* end namespace reduction */

extern reduction::ReductionManager* theReduction();

} /* end namespace vt */

#include "reduction/reduction_manager.impl.h"

#endif /*INCLUDED_REDUCTION_REDUCTION_MANAGER_H*/
