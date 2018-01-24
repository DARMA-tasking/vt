
#if !defined INCLUDED_COLLECTIVE_COLLECTIVE_ALG_BARRIER_STATE_H
#define INCLUDED_COLLECTIVE_COLLECTIVE_ALG_BARRIER_STATE_H

#include "config.h"

namespace vt { namespace collective {

struct BarrierState {
  BarrierType barrier;

  int recv_event_count = 0;
  bool is_wait = false;
  bool is_named = false;
  bool released = false;

  ActionType cont_action = nullptr;

  BarrierState(
    bool const& in_is_named, BarrierType const& in_barrier, bool const& in_is_wait
  ) : barrier(in_barrier), is_wait(in_is_wait), is_named(in_is_named)
  { }
};

}} /* end namespace vt::collective */

#endif /*INCLUDED_COLLECTIVE_COLLECTIVE_ALG_BARRIER_STATE_H*/