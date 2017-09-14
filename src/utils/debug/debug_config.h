
#if !defined INCLUDED_DEBUG_CONFIG
#define INCLUDED_DEBUG_CONFIG

#include "tpl/meld/meld_meldheaders.h"
#include "configs/features/features_featureswitch.h"
#include "configs/features/features_defines.h"
#include "configs/features/features_metafeatures.h"
#include "configs/features/features_enableif.h"
#include "debug_printconst.h"

/* Printers for configurations */

#define backend_option_print_if_enabled(key, value)           \
  backend_enable_if(key, backend_option_printer(key,value))

#define backend_option_printer(key, value)      \
  "\t" #key ": " debug_pretty_print(key) "\n"

#define backend_option_printer_test(key, value) \
  backend_enable_if_else(key, on#key, off#key)
#define backend_option_printer_test_on(key, value) \
  backend_enable_if(key, backend_option_printer(key,value))

#define backend_print_all_options(print_all, key_value_map...)          \
  meld_if_stmt(print_all)(                                              \
    _meld_eval_32768(                                                   \
      meld_print_kv(                                                    \
        backend_option_printer,                                         \
        key_value_map                                                   \
      )                                                                 \
    )                                                                   \
  )(                                                                    \
    _meld_eval_32768(                                                   \
      meld_print_kv(                                                    \
        backend_option_printer_test_on,                                 \
        key_value_map                                                   \
      )                                                                 \
    )                                                                   \
  )

#define backend_print_all_features(print_all)         \
  backend_print_all_options(                          \
    print_all, debug_list_features                    \
  )                                                   \

#define backend_print_all_debug_modes(print_all)      \
  backend_print_all_options(                          \
    print_all, meld_eval_2(debug_list_debug_modes)    \
  )

#define debug_check_if_equal(tok1, tok2)        \
  meld_token_compare(tok1,tok2)

#define debug_pretty_print_foreach(opt, key_value_map...)               \
  meld_eval_32(                                                         \
    meld_lookup_key(                                                    \
      debug_check_if_equal,                                             \
      opt, key_value_map                                                \
    )                                                                   \
  )

#define debug_lookup_is_printer(opt, key_value_map...)                  \
  meld_eval_32(                                                         \
    meld_lookup_else(                                                   \
      debug_check_if_equal,                                             \
      opt, 0, key_value_map                                             \
    )                                                                   \
  )

#define debug_pretty_print(opt)                    \
  debug_pretty_print_foreach(                      \
    opt, meld_eval_2(debug_list_all)               \
  )                                                \

/*
 * Contexts to print function generation
 */

#define debug_list_contexts_printfn_kv                                  \
  debug_list_holder(                                                    \
    node,                  node,                                        \
    unknown,               unknown                                      \
  )

#define debug_check_context(ctx1, ctx2)         \
  meld_token_compare(ctx1, ctx2)

#define debug_check_context_transform(key, value) \
  debug_print_ ## value

#define debug_print_context_foreach(opt, key_value_map...)              \
  meld_eval_32(                                                         \
    meld_transform_key(                                                 \
      debug_check_context, opt,                                         \
      debug_check_context_transform, key_value_map                      \
    )                                                                   \
  )

#define debug_print_context(feature, opt, arg...)     \
  debug_print_context_foreach(                        \
    opt, meld_eval_2(debug_list_contexts_printfn_kv)  \
  )(feature, arg)

#endif  /*INCLUDED_DEBUG_CONFIG*/
