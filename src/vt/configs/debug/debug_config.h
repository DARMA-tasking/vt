
#if !defined INCLUDED_DEBUG_CONFIG
#define INCLUDED_DEBUG_CONFIG

#include "meld_headers.h"
#include "configs/features/features_featureswitch.h"
#include "configs/features/features_defines.h"
#include "configs/features/features_metafeatures.h"
#include "configs/features/features_enableif.h"
#include "configs/debug/debug_printconst.h"

/* Printers for configurations */

#define backend_printer_value(key, value) "\t" value "\n"

#define backend_option_printer_sane(test, key, value)          \
  meld_if_stmt(test)(                                          \
     backend_enable_if(key, backend_printer_value(key,value))  \
  )(                                                           \
     backend_printer_value(key,value)                          \
   )

#define backend_print_all_options_sane(test, printer, key_value_map...) \
  _meld_eval_32768(                                                     \
    meld_print_kv_state(                                                \
      printer, test, key_value_map                                      \
    )                                                                   \
  )                                                                     \

#define backend_print(print_all, printer, options...) \
  backend_print_all_options_sane(                     \
    print_all, printer, options                       \
  )                                                   \

#define backend_print_features(print_all)             \
  backend_print_list(print_all, debug_list_features)
#define backend_print_debug(print_all)                  \
  backend_print_list(print_all, debug_list_debug_modes)

#define backend_print_list(print_all, list...)                  \
  backend_print(                                                \
    print_all, backend_option_printer_sane, list                \
  )                                                             \

#define backend_option_print_if_enabled(key, value)           \
  backend_enable_if(key, backend_option_printer(key,value))

#define backend_option_printer(key, value)      \
  "\t" #key ": " debug_pretty_print(key) "\n"

#define backend_option_printer_test(key, value) \
  backend_enable_if_else(key, on#key, off#key)
#define backend_option_printer_test_on(key, value) \
  backend_enable_if(key, backend_option_printer(key,value))
#define backend_option_test_on(printer, key, value)         \
  backend_enable_if(key, printer(key,value))

#define backend_print_all_options(print_all, printer, key_value_map...) \
  meld_if_stmt(print_all)(                                              \
    _meld_eval_4096(                                                    \
      meld_print_kv(                                                    \
        printer,                                                        \
        key_value_map                                                   \
      )                                                                 \
    )                                                                   \
  )(                                                                    \
    _meld_eval_4096(                                                    \
      meld_print_kv(                                                    \
        backend_option_printer_test_on,                                 \
        key_value_map                                                   \
      )                                                                 \
    )                                                                   \
  )

#define backend_print_all_debug_modes(print_all)      \
  backend_print_all_options(                          \
    print_all, debug_list_debug_modes                 \
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

#define remove_any_replace_(fn, option, listoptions...)                 \
  meld_eval(                                                            \
    meld_map_with_trans(                                                \
      fn, invert_current_map_state,                                     \
      meld_to_bool(bool), option, listoptions                           \
    )                                                                   \
  )
#define invert_current_map_state(state) meld_not(state)
#define replace_even_previous(list_element, map_bool_state, not_last)   \
  meld_if_stmt(map_bool_state)(                                         \
    list_element,                                                       \
    meld_if_stmt(not_last)(                                             \
      list_element,                                                     \
    )(                                                                  \
      list_element                                                      \
    )                                                                   \
  )()
#define replace_only_key(list_element, map_bool_state, not_last)        \
  meld_if_stmt(map_bool_state)(                                         \
    meld_if_stmt(not_last)(                                             \
      list_element,                                                     \
    )(                                                                  \
      list_element                                                      \
    )                                                                   \
  )()
#define remove_even_strings(options...)               \
  remove_any_replace_(replace_even_previous, options)
#define remove_all_strings(options...)                \
  remove_any_replace_(replace_only_key, options)

#define debug_list_contexts_printfn_kv          \
  remove_even_strings(debug_list_contexts)

#define debug_list_subclass_printfn_kv          \
  remove_even_strings(debug_list_subclass)

#define debug_check_context(ctx1, ctx2)         \
  meld_token_compare(ctx1, ctx2)

#define debug_check_context_transform(key, value) \
  debug_print_ ## value

#define debug_print_context_foreach(config, opt, key_value_map...)      \
  meld_eval_32(                                                         \
    meld_transform_key(                                                 \
      debug_check_context, opt,                                         \
      debug_check_context_transform, key_value_map                      \
    )                                                                   \
  )

#define debug_print_context(config, feature, opt, arg...)     \
  debug_print_context_foreach(                                \
    config, opt, meld_eval_2(debug_list_contexts_printfn_kv)  \
  )(feature, arg)

#define filter_enabled(options...) filter_enabled_(options)
#define filter_enabled_(option, listoptions...)                         \
  _meld_eval_32768(                                                     \
    meld_map_with_trans_m(                                              \
      output_if_enabled, invert_filter_map_state,                       \
      meld_to_bool(bool), option, listoptions                           \
    )                                                                   \
  )
#define invert_filter_map_state(state) meld_not(state)
#define output_if_enabled(list_element, map_bool_state, prev)           \
  meld_if_stmt(map_bool_state)(                                         \
  )(                                                                    \
    meld_if_stmt(                                                       \
      check(prev, backend_feature_configuration)                        \
    )(                                                                  \
      "\t" #list_element "\n"                                           \
    )()                                                                 \
  )

#define debug_key_only_features remove_all_strings(debug_list_features)

#define debug_find_value_kv(opt, key_value_map...)                      \
  meld_eval_32(                                                         \
    meld_lookup_key(                                                    \
      debug_check_if_equal, opt, key_value_map                          \
    )                                                                   \
  )

#define find_value_enabled(feature_list, options...)  \
  find_value_enabled_(feature_list, options)
#define find_value_enabled_(feature_list, option, listoptions...)       \
  _meld_eval_8192(                                                      \
    meld_map_find_other(                                                \
      find_if_enabled, feature_list, option, listoptions                \
    )                                                                   \
  )
#define find_if_enabled(list_element, kv_list)                          \
  "\t"                                                                  \
  debug_find_value_kv(list_element, backend_str_join(kv_list, debug_))  \
  "\n"

//find_value_enabled(list_debug_modes, backend_debug_modes)

#define to_tuple(list...) (list)
#define to_list(tuple...) listify tuple
#define listify(list...) list

#define find_sorted_expand_2(key, value, key2, rest...)       \
  meld_if_stmt(debug_check_if_equal(key, key2))(              \
    #value                                                    \
  )()

#define find_sorted_expand(key, value, rest...) \
  find_sorted_expand_2(key,value,rest)

#define find_sorted(key, value, feature_tuple)          \
  find_sorted_expand(key, value, to_list(feature_tuple))

#define recons_list(opts...) to_tuple(opts)

#define remove_if_found_expand_2(list_element, kv, kv_list...)  \
  meld_if_stmt(meld_token_compare(list_element, kv))(           \
    recons_list(kv_list)                                        \
  )(                                                            \
    recons_list(kv, kv_list)                                    \
  )                                                             \

#define remove_if_found_expand(list_element, kv_list...)  \
  remove_if_found_expand_2(list_element, kv_list)

#define remove_if_found(list_element, feature_tuple)            \
  remove_if_found_expand(list_element, to_list(feature_tuple))

#define find_value_sorted(feature_list, options...) \
  find_value_sorted_(feature_list, options)
#define find_value_sorted_(feature_list, option, listoptions...)        \
  _meld_eval_8192(                                                      \
    meld_map_find_sorted(                                               \
      find_sorted, remove_if_found,                                     \
      feature_list,                                                     \
      option, listoptions                                               \
    )                                                                   \
  )

#define backend_print_all_features(print_all)         \
  find_value_enabled(list_features, backend_features)

#endif  /*INCLUDED_DEBUG_CONFIG*/
