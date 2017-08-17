
#if ! defined __RUNTIME_TRANSPORT_TERM_STATE__
#define __RUNTIME_TRANSPORT_TERM_STATE__

#include "common.h"

namespace runtime { namespace term {

using term_counter_t = int64_t;

struct TermState {
  // four-counter method
  term_counter_t l_prod = 0, l_cons = 0;

  term_counter_t g_prod1 = 0, g_cons1 = 0;
  term_counter_t g_prod2 = 0, g_cons2 = 0;

  // when this is equal to num_children+1, ready to propgate
  int recv_event_count = 0, recv_wave_count = 0;

  bool propagate = true;
};

}} //end namespace runtime::term

#endif /*__RUNTIME_TRANSPORT_TERM_STATE__*/
