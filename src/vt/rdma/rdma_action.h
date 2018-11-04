
#if !defined INCLUDED_RDMA_RDMA_ACTION_H
#define INCLUDED_RDMA_RDMA_ACTION_H

#include "vt/config.h"
#include "vt/rdma/rdma_common.h"

#include <sstream>
#include <iostream>

namespace vt { namespace rdma {

struct Action {
  using ActionCountType = int;

  ActionCountType num_waiting = 0;
  ActionType action_to_trigger = nullptr;

  Action(
    ActionCountType const& num_waiting_in, ActionType in_action_to_trigger
  ) : num_waiting(num_waiting_in), action_to_trigger(in_action_to_trigger)
  { }

  void addDep() {
    num_waiting++;
  }

  void release() {
    num_waiting--;
    if (num_waiting == 0) {
      if (action_to_trigger) {
        action_to_trigger();
      }
      delete this;
    }
  }
};

}} //end namespace vt::rdma

#endif /*INCLUDED_RDMA_RDMA_ACTION_H*/
