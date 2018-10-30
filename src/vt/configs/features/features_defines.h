
#if !defined INCLUDED_FEATURES_DEFINES
#define INCLUDED_FEATURES_DEFINES

#include "meld_headers.h"

/*
 * All the defined features/options for debugging and backend enable-ifs
 */

// backend features, add any new ones to this list
#define debug_no_feature(x) x
#define debug_bit_check_overflow(x) x
#define debug_trace_enabled(x) x
#define debug_detector(x) x
#define debug_lblite(x) x
#define debug_openmp(x) x
#define debug_production(x) x
#define debug_stdthread(x) x
#define debug_parserdes(x) x
#define debug_print_term_msgs(x) x
#define debug_default_threading(x) x
#define debug_no_pool_alloc_env(x) x
#define debug_memory_pool(x) x

// distinct modes for debug
#define debug_none(x) x
#define debug_gen(x) x
#define debug_runtime(x) x
#define debug_active(x) x
#define debug_term(x) x
#define debug_barrier(x) x
#define debug_event(x) x
#define debug_pipe(x) x
#define debug_pool(x) x
#define debug_reduce(x) x
#define debug_rdma(x) x
#define debug_rdma_channel(x) x
#define debug_rdma_state(x) x
#define debug_param(x) x
#define debug_handler(x) x
#define debug_hierlb(x) x
#define debug_scatter(x) x
#define debug_sequence(x) x
#define debug_sequence_vrt(x) x
#define debug_serial_msg(x) x
#define debug_trace(x) x
#define debug_location(x) x
#define debug_lb(x) x
#define debug_vrt(x) x
#define debug_vrt_coll(x) x
#define debug_worker(x) x
#define debug_group(x) x
#define debug_broadcast(x) x

// contextual modes for debug
#define debug_node(x) x
#define debug_unknown(x) x

// global modes for debug
#define debug_flush(x) x
#define debug_startup(x) x
#define debug_line_file(x) x
#define debug_function(x) x

// subclass modes
#define debug_verbose(x) x
#define debug_verbose_2(x) x

#endif  /*INCLUDED_FEATURES_DEFINES*/
