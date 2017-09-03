
#if ! defined __BACKEND_META_FEATURES__
#define __BACKEND_META_FEATURES__

#include "meld.h"
#include "backend_feature_defines.h"

// Meta-features and debug and turn combinations on
#define debug_meta_all                                                  \
  gen, active, termination, event, pool, rdma, rdma_channel,            \
  rdma_state, handler, flush

// @todo: needed anymore?
#define debug_sentinel_token       debug_sentinel_token

#endif
