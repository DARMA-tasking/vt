
#if !defined INCLUDED_FEATURES_META_FEATURES
#define INCLUDED_FEATURES_META_FEATURES

#include "meld_headers.h"
#include "features_defines.h"

// Meta-features and debug and turn combinations on
#define debug_meta_all                                                  \
  gen, runtime, active, term, barrier, event, pool, rdma, rdma_channel, \
  rdma_state, handler, group, location, broadcast, flush

// @todo: needed anymore?
#define debug_sentinel_token       debug_sentinel_token

#endif  /*INCLUDED_FEATURES_META_FEATURES*/
