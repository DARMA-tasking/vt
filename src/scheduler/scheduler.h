
#if ! defined __RUNTIME_TRANSPORT_SCHEDULER__
#define __RUNTIME_TRANSPORT_SCHEDULER__

#include "common.h"

namespace runtime { namespace scheduler {

struct Scheduler {

  static void
  check_term_single_node();

  static void
  scheduler();

  static void
  scheduler_forever();

};

}} //end namespace runtime::scheduler

namespace runtime {

void
run_scheduler();

}  //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_SCHEDULER__*/
