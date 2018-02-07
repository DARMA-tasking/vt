
#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_STATE_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_STATE_H

#include "config.h"
#include "collective/reduce/reduce_msg.h"

#include <vector>

namespace vt { namespace collective { namespace reduce {

struct ReduceState {
  ReduceState(
      TagType const& in_tag_, EpochType const& in_epoch_
  ) : tag_(in_tag_), epoch_(in_epoch_)
  { }

  std::vector<ReduceMsg*> msgs;
  //int recv_event_count = 0;
  TagType tag_ = no_tag;
  EpochType epoch_ = no_epoch;
};

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_STATE_H*/
