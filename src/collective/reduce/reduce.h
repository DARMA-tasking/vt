
#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_H

#include "config.h"
#include "collective/reduce/reduce_state.h"
#include "collective/reduce/reduce_msg.h"
#include "messaging/active.h"
#include "activefn/activefn.h"
#include "messaging/message.h"
#include "collective/tree/tree.h"
#include "utils/hash/hash_tuple.h"

#include <tuple>
#include <unordered_map>
#include <cassert>

namespace vt { namespace collective { namespace reduce {

struct Reduce : virtual collective::tree::Tree {
  using ReduceIdentifierType = std::tuple<TagType, EpochType>;
  using ReduceStateType = ReduceState;

  Reduce();

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

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_H*/
