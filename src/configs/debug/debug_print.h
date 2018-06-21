
#if !defined INCLUDED_DEBUG_PRINT
#define INCLUDED_DEBUG_PRINT

#include "debug_config.h"

#include <fmt/format.h>

#define debug_flush_to_out(config, stdout)       \
  debug_cond_enabled(config, flush, fflush(stdout))

#define debug_file_line_args ,__FILE__, __LINE__

#define debug_file_arg                                \
  debug_test(backend,line_file, debug_file_line_args, )
#define debug_file_fmt                          \
  debug_test(backend,line_file, "{}:{} ", )

#define debug_decorated_prefix(debug_stamp, debug_type)       \
  "[{}] " debug_stamp " " debug_pretty_print(debug_type) ": "

#define debug_decorated(                                                \
  PRINTER,                                                              \
  debug_type, debug_stamp,                                              \
  proc,                                                                 \
  has_ctx_1, c1_fmt, c1_arg,                                            \
  has_ctx_2, c2_fmt, c2_arg,                                            \
  main_fmt, main_arg...                                                 \
)                                                                       \
  do {                                                                  \
    meld_if_stmt(meld_to_bool(has_ctx_1))(                              \
      meld_if_stmt(meld_to_bool(has_ctx_2))(                            \
        PRINTER(                                                        \
          debug_decorated_prefix(debug_stamp, debug_type)               \
          c1_fmt " " c2_fmt ": " debug_file_fmt main_fmt,               \
          proc, c1_arg, c2_arg debug_file_arg                           \
          meld_if_stmt(                                                 \
            meld_to_bool(_meld_is_empty(main_arg))                      \
          )()(,main_arg)                                                \
        );                                                              \
      )(                                                                \
        PRINTER(                                                        \
          debug_decorated_prefix(debug_stamp, debug_type)               \
          c1_fmt ": " debug_file_fmt main_fmt,                          \
          proc, c1_arg debug_file_arg                                   \
          meld_if_stmt(                                                 \
            meld_to_bool(_meld_is_empty(main_arg))                      \
          )()(,main_arg)                                                \
        );                                                              \
      )                                                                 \
    )(                                                                  \
      PRINTER(                                                          \
        debug_decorated_prefix(debug_stamp, debug_type)                 \
        debug_file_fmt main_fmt,                                        \
        proc debug_file_arg                                             \
        meld_if_stmt(                                                   \
          meld_to_bool(_meld_is_empty(main_arg))                        \
        )()(,main_arg)                                                  \
      );                                                                \
    )                                                                   \
    debug_flush_to_out(config, stdout);                                 \
  } while (0);

#define ctx_true 1
#define ctx_false 0

#define print_ctx_node   (                                              \
    ::vt::theContext() ?                                                \
      (::vt::theContext()->getNode()) :                                 \
      static_cast<NodeType>(-1)                                         \
  )
#define print_ctx_worker   (                                            \
    ::vt::theContext() ?                                                \
      (::vt::theContext()->getWorker()) :                               \
      static_cast<NodeType>(-1)                                         \
  )
#define print_ctx_comm_worker                                           \
  (print_ctx_worker == ::vt::worker_id_comm_thread) ?                   \
  ::vt::comm_debug_print :                                              \
  print_ctx_worker

#define debug_virtual(\
  debug_type, \
  has_c1, c1_fmt, c1_arg, \
  has_c2, c2_fmt, c2_arg, \
  main_fmt, main_arg...                                                 \
)                                                                       \
  debug_virtual_pe(                                                     \
    debug_type, print_ctx_node,                                         \
    has_c1, c1_fmt, c1_arg,                                             \
    has_c2, c2_fmt, c2_arg,                                             \
    main_fmt, ##main_arg                                                \
  )                                                                     \

#define debug_virtual_pe(\
  debug_type, proc, \
  has_c1, c1_fmt, c1_arg, \
  has_c2, c2_fmt, c2_arg, \
  main_fmt, main_arg...                                                 \
)                                                                       \
  debug_decorated(                                                      \
    ::fmt::print, debug_type, "VT", proc,                               \
    has_c1, c1_fmt, c1_arg,                                             \
    has_c2, c2_fmt, c2_arg,                                             \
    main_fmt, ##main_arg                                                \
  )

#define debug_virtual_ctx_2(\
  debug_type, \
  c1_fmt, c1_arg, \
  c2_fmt, c2_arg, \
  main_fmt, main_arg...                                                 \
)                                                                       \
  debug_virtual(                                                        \
    debug_type,                                                         \
    ctx_true, c1_fmt, c1_arg,                                           \
    ctx_true, c2_fmt, c2_arg,                                           \
    main_fmt, ##main_arg                                                \
  )                                                                     \

#define debug_virtual_ctx_1(\
  debug_type, \
  c1_fmt, c1_arg, \
  main_fmt, main_arg...                                                 \
)                                                                       \
  debug_virtual(                                                        \
    debug_type,                                                         \
    ctx_true,  c1_fmt, c1_arg,                                          \
    ctx_false, "{}",   "",                                              \
    main_fmt, ##main_arg                                                \
  )                                                                     \

#define debug_virtual_proc_ctx_none(\
  debug_type, proc, \
  main_fmt, main_arg...                                                 \
)                                                                       \
  debug_virtual_pe(                                                     \
    debug_type, proc,                                                   \
    ctx_false, "{}", "",                                                \
    ctx_false, "{}", "",                                                \
    main_fmt, ##main_arg                                                \
  )                                                                     \

#define debug_virtual_ctx_none(\
  debug_type, \
  main_fmt, main_arg...                                                 \
)                                                                       \
  debug_virtual_proc_ctx_none(                                          \
    debug_type, print_ctx_node, main_fmt, ##main_arg                    \
  )

// define a set of debug print variants that are each unsed by some set of modes
#define debug_print_regular(debug_type, main_fmt, main_arg...)  \
  debug_virtual_ctx_none(debug_type, main_fmt, main_arg)

// #define debug_print_array(debug_type, main_fmt, main_arg...)  \
//   debug_virtual_ctx_1(debug_type, "idx={}", thisIndex, main_fmt, ##main_arg)

// #define debug_print_aoth(debug_type, main_fmt, main_arg...)             \
//   debug_virtual_ctx_1(debug_type, "idx={}", this_index, main_fmt, main_arg)

#define debug_print_node(debug_type, main_fmt, main_arg...)             \
  debug_virtual_ctx_2(                                                  \
    debug_type,                                                         \
    "node={}",   print_ctx_node,                                        \
    "worker={}", print_ctx_comm_worker,                                 \
    main_fmt, main_arg                                                  \
  )

#define debug_print_unknown(debug_type, main_fmt, main_arg...)  \
  debug_virtual_proc_ctx_none(debug_type, -1, main_fmt, main_arg)

// #define debug_print_pe(debug_type, proc, main_fmt, main_arg...)   \
//   debug_virtual_proc_ctx_none(debug_type, proc, main_fmt, main_arg)

#define debug_print_uid(debug_type, main_fmt, main_arg...)           \
  debug_virtual_ctx_2(                                               \
    debug_type,                                                      \
    "idx={}", thisIndex,                                             \
    "uid={}", task_collection_id.unique_id,                          \
    main_fmt, main_arg                                               \
  )

#define verbose_info(debug_type, main_fmt, main_arg...) \
  debug_virtual_ctx_none(debug_type, main_fmt, main_arg)
#define verbose_info_pe(debug_type, proc, main_fmt, main_arg...)  \
  debug_virtual_proc_ctx_none(debug_type, proc, main_fmt, main_arg)

#define virtual_fatal_error(str)                  \
  do {                                            \
    ::fmt::print(stderr, str);                    \
    exit(229);                                    \
  } while (0);

#define debug_print_locale(feature, fst_arg_as_pe, arg...) \
  debug_print_pe(feature, fst_arg_as_pe, arg)

#define debug_print(feature, maybe_ctx, arg...)                         \
  meld_eval_16(                                                         \
    debug_print_recur_call(backend_debug,feature,maybe_ctx,arg)         \
  )

#define debug_print_normal(config, feature, ctx, arg...)                \
  debug_cond_enabled(                                                   \
    config, feature,                                                    \
    debug_print_context(config, feature, ctx, arg)                      \
  )

#define debug_print_handle_subclass(config, sub, feature, ctx, arg...)  \
  debug_cond_enabled_else(                                              \
    config,                                                             \
    feature,                                                            \
    debug_print_context(config, feature, ctx, arg),                     \
    debug_cond_enabled(                                                 \
      config,                                                           \
      sub,                                                              \
      debug_print_context(config, feature, ctx, arg)                    \
    )                                                                   \
  )                                                                     \

#define debug_print_recur_inner(config, feature, maybe_ctx, arg...)     \
  meld_if_stmt(                                                         \
    debug_lookup_is_printer(                                            \
      maybe_ctx, meld_eval_2(debug_list_contexts_printfn_kv)            \
    )                                                                   \
  )(                                                                    \
    debug_print_normal(config, feature, maybe_ctx, arg)                 \
  )(                                                                    \
    debug_cond_enabled_else(                                            \
      config,                                                           \
      feature,                                                          \
      debug_print_recur_2(config, maybe_ctx, arg),                      \
      debug_cond_enabled(                                               \
        config,                                                         \
        maybe_ctx,                                                      \
        debug_print_context(config, maybe_ctx, arg)                     \
      )                                                                 \
    )                                                                   \
  )

#define debug_print_recur(config, feature, maybe_ctx, arg...)           \
  meld_if_stmt(                                                         \
    debug_lookup_is_printer(                                            \
      feature, meld_eval_2(debug_list_subclass_printfn_kv)              \
    )                                                                   \
  )(                                                                    \
    debug_cond_enabled(                                                 \
      config, feature,                                                  \
      debug_print_normal(config, maybe_ctx, arg)                        \
    )                                                                   \
  )(                                                                    \
    debug_print_recur_inner(config, feature, maybe_ctx, arg)            \
  )

#define debug_print_recur_2(config, feature, maybe_ctx, arg...)         \
  meld_if_stmt(                                                         \
    debug_lookup_is_printer(                                            \
      maybe_ctx, meld_eval_2(debug_list_contexts_printfn_kv)            \
    )                                                                   \
  )(                                                                    \
    debug_print_context(config, feature, maybe_ctx, arg)                \
  )(                                                                    \
    debug_print_context(config, maybe_ctx, arg)                         \
  )

#define _debug__print_recur() debug_print_recur
#define debug_print_recur_call debug_print_recur

// check all special subclass tokens in the future
#define debug_print_check_subclass(config, feature, opt, arg...)  \
  meld_if_stmt(                                                   \
    meld_token_compare(feature, verbose)                          \
  )(                                                              \
    debug_print_handle_subclass(feature, opt, arg)                \
  )(                                                              \
    debug_print_normal(config, feature, opt, arg)                 \
  )

#if debug_force_enabled
  //#warning "Debug force is enabled"
  #define debug_print_force(feature, opt, arg...)         \
    debug_print_context(backend_debug, feature, opt, arg)
#else
  //#warning "Debug force is not enabled"
  #define debug_print_force debug_print
#endif

#define backend_debug_print debug_print

#endif  /*INCLUDED_DEBUG_PRINT*/
