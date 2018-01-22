
#if !defined INCLUDED_REDUCTION_REDUCTION_STATE_H
#define INCLUDED_REDUCTION_REDUCTION_STATE_H

#include "config.h"
#include "reduction/reduce_msg.h"

#include <vector>

namespace vt { namespace reduction {

struct State {
  State(
    TagType const& in_tag_, EpochType const& in_epoch_
  ) : tag_(in_tag_), epoch_(in_epoch_)
  { }

  std::vector<ReduceMsg*> msgs;
  //int recv_event_count = 0;
  TagType tag_ = no_tag;
  EpochType epoch_ = no_epoch;
};

}} /* end namespace vt::reduction */

#endif /*INCLUDED_REDUCTION_REDUCTION_STATE_H*/
