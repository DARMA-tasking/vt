
#if !defined INCLUDED_COLLECTIVE_BARRIER_BARRIER_MSG_H
#define INCLUDED_COLLECTIVE_BARRIER_BARRIER_MSG_H

#include "vt/config.h"
#include "vt/messaging/message.h"

namespace vt { namespace collective { namespace barrier {

struct BarrierMsg : vt::EpochMessage {
  bool is_named, is_wait, skip_term = false;
  BarrierType barrier;

  BarrierMsg(
    bool const& in_is_named, BarrierType const& in_barrier,
    bool const& in_is_wait
  ) : EpochMessage(), is_named(in_is_named), is_wait(in_is_wait),
      skip_term(false), barrier(in_barrier)
  { }
};

}}} /* end namespace vt::collective::barrier */

#endif /*INCLUDED_COLLECTIVE_BARRIER_BARRIER_MSG_H*/
