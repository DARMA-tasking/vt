
#if !defined INCLUDED_VTEF_META_FEATURES
#define INCLUDED_VTEF_META_FEATURES

#include "vte/vtem/vtem_meld.h"
#include "vtef_defines.h"

// Meta-features and debug and turn combinations on
#define debug_meta_all                                                  \
  gen, active, termination, event, pool, rdma, rdma_channel,            \
  rdma_state, handler, flush

// @todo: needed anymore?
#define debug_sentinel_token       debug_sentinel_token

#endif
