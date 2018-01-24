
#if !defined INCLUDED_COLLECTIVE_COLLECTIVE_ALG_REDUCE_STATE_H
#define INCLUDED_COLLECTIVE_COLLECTIVE_ALG_REDUCE_STATE_H

#include "config.h"
#include "collective_alg_reduce_msg.h"

#include <vector>

namespace vt { namespace collective {

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

}} /* end namespace vt::collective */

#endif /*INCLUDED_COLLECTIVE_COLLECTIVE_ALG_REDUCE_STATE_H*/