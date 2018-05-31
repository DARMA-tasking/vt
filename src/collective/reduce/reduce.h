
#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_H

#include "config.h"
#include "collective/reduce/reduce.fwd.h"
#include "collective/reduce/reduce_hash.h"
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
#include <cstdint>

namespace vt { namespace collective { namespace reduce {

struct Reduce : virtual collective::tree::Tree {
  using ReduceStateType = ReduceState;
  using ReduceNumType = ReduceState::ReduceNumType;

  Reduce();

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EpochType reduce(
    NodeType const& root, MessageT* const msg, TagType const& tag = no_tag,
    EpochType const& epoch = no_epoch, ReduceNumType const& num_contrib = 1,
    VirtualProxyType const& proxy = no_vrt_proxy
  );

  template <typename MessageT>
  void reduceAddMsg(MessageT* msg, ReduceNumType const& num_contrib = -1);

  template <typename MessageT>
  void reduceNewMsg(MessageT* msg);

  template <typename MessageT>
  static void reduceRootRecv(MessageT* msg);
  template <typename MessageT>
  static void reduceUp(MessageT* msg);

private:
  std::unordered_map<ReduceEpochLookupType,EpochType> next_epoch_for_tag_;
  std::unordered_map<ReduceIdentifierType,ReduceStateType> live_reductions_;
};

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_H*/
