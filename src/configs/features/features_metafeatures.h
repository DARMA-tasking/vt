
#if !defined INCLUDED_FEATURES_META_FEATURES
#define INCLUDED_FEATURES_META_FEATURES

#include "meld_headers.h"
#include "features_defines.h"

// Meta-features and debug and turn combinations on
#define debug_meta_all                                                  \
  active, barrier, broadcast, event, gen, group, handler, lb, location, \
  param, pool, rdma, rdma, rdma, runtime, sequence, sequence, serial,   \
  term, trace, vrt, vrt, worker

// @todo: needed anymore?
#define debug_sentinel_token       debug_sentinel_token

#endif  /*INCLUDED_FEATURES_META_FEATURES*/
