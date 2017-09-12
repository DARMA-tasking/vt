
#if ! defined __RUNTIME_TRANSPORT_RDMA_ACTION__
#define __RUNTIME_TRANSPORT_RDMA_ACTION__

#include "common.h"
#include "rdma_common.h"

#include <sstream>
#include <iostream>

namespace runtime { namespace rdma {

struct Action {
  using ActionCountType = int;

  ActionCountType num_waiting = 0;
  ActionType action_to_trigger = nullptr;

  Action(
    ActionCountType const& num_waiting_in, ActionType in_action_to_trigger
  ) : num_waiting(num_waiting_in), action_to_trigger(in_action_to_trigger)
  { }

  void add_dep() {
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

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_ACTION__*/
