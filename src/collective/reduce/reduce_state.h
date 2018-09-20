
#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_STATE_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_STATE_H

#include "config.h"
#include "collective/reduce/reduce_msg.h"
#include "messaging/message.h"

#include <vector>
#include <cstdint>

namespace vt { namespace collective { namespace reduce {

struct ReduceState {
  using ReduceNumType = int32_t;
  using ReduceVecType = std::vector<MsgSharedPtr<ReduceMsg>>;

  ReduceState(
    TagType const& in_tag_, EpochType const& in_epoch_,
    ReduceNumType const& in_num_contrib
  ) : tag_(in_tag_), epoch_(in_epoch_), num_contrib_(in_num_contrib)
  { }

  ReduceVecType msgs               = {};
  TagType tag_                     = no_tag;
  EpochType epoch_                 = no_epoch;
  ReduceNumType num_contrib_       = 1;
  ReduceNumType num_local_contrib_ = 0;
  HandlerType combine_handler_     = uninitialized_handler;
  NodeType reduce_root_            = uninitialized_destination;
};

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_STATE_H*/
