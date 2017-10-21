
#if !defined INCLUDED_FEATURES_DEFINES
#define INCLUDED_FEATURES_DEFINES

#include "meld/meld_headers.h"

/*
 * All the defined features/options for debugging and backend enable-ifs
 */

// backend features, add any new ones to this list
#define debug_no_feature(x) x
#define debug_bit_check_overflow(x) x
#define debug_trace_enabled(x) x
#define debug_detector(x) x
#define debug_openmp(x) x

// distinct modes for debug
#define debug_none(x) x
#define debug_gen(x) x
#define debug_runtime(x) x
#define debug_active(x) x
#define debug_term(x) x
#define debug_barrier(x) x
#define debug_event(x) x
#define debug_pool(x) x
#define debug_rdma(x) x
#define debug_rdma_channel(x) x
#define debug_rdma_state(x) x
#define debug_param(x) x
#define debug_handler(x) x
#define debug_sequence(x) x
#define debug_sequence_vrt(x) x
#define debug_trace(x) x
#define debug_location(x) x
#define debug_vrt(x) x
#define debug_worker(x) x

// contextual modes for debug
#define debug_node(x) x
#define debug_unknown(x) x

// global modes for debug
#define debug_flush(x) x
#define debug_startup(x) x
#define debug_line_file(x) x
#define debug_function(x) x

#endif  /*INCLUDED_FEATURES_DEFINES*/
