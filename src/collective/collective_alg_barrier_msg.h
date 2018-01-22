
#if !defined INCLUDED_COLLECTIVE_COLLECTIVE_ALG_BARRIER_MSG_H
#define INCLUDED_COLLECTIVE_COLLECTIVE_ALG_BARRIER_MSG_H

#include "config.h"
#include "messaging/message.h"

namespace vt { namespace collective {

struct BarrierMsg : vt::ShortMessage {
  bool is_named, is_wait, skip_term = false;
  BarrierType barrier;

  BarrierMsg(
    bool const& in_is_named, BarrierType const& in_barrier, bool const& in_is_wait
  )
    : ShortMessage(), is_named(in_is_named), is_wait(in_is_wait),
      skip_term(false), barrier(in_barrier)
  { }
};

}} //end namespace vt::barrier

#endif /*INCLUDED_COLLECTIVE_COLLECTIVE_ALG_BARRIER_MSG_H*/
