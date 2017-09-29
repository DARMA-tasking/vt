
#if !defined INCLUDED_FEATURES_ENABLE_IF
#define INCLUDED_FEATURES_ENABLE_IF

#include "meld/meld_headers.h"
#include "features_featureswitch.h"
#include "features_defines.h"

#define debug_options_build(options...) options
#define debug_local_options_on(options...) options

// #define debug_resolve_op(option) debug_ ## option(option),
// #define debug_resolve_options(options...)             \
//   meld_meta_map(meld_map, debug_resolve_op, ##options)

#define backend_options_on(arg...) arg

#define debug_options_on(options...)                              \
  debug_options_build(options)                                    \

#define backend_configuration debug_options_on(                 \
    backend_features,                                           \
    backend_debug_contexts,                                     \
    backend_debug_modes,                                        \
    backend_defaults                                            \
  )

/* Test functions toc check if options are enabled */

#define debug_test_option_or(option, to_test)   \
  meld_token_compare(option, to_test),
#define debug_test_option_shortcut(opt1, opt2)  \
  meld_token_compare(opt1, opt2)

// Use the meld's logical or to determine if any option is on
#define check_enabled_or(test_option, options...)                       \
  meld_fold_or(                                                         \
    meld_eval(meld_map_with(debug_test_option, test_option, options))   \
  )

// Yield 1/0 and shortcut boolean check
#define check_enabled_shortcut(test_option, options...)                 \
  meld_eval(                                                            \
    meld_check_enabled(                                                 \
      debug_test_option_shortcut, test_option, options                  \
    )                                                                   \
  )

#define debug_check_enabled_shortcut_impl(test_option, options...)  \
  check_enabled_shortcut(test_option, options)
#define debug_check_enabled_or_impl(test_option, options...)  \
  check_enabled_shortcut(test_option, options)
#define debug_check_enabled_shortcut(test_option) \
  debug_check_enabled_shortcut_impl(test_option, backend_configuration)
#define debug_check_enabled_or(test_option)       \
  debug_check_enabled_or_impl(test_option, backend_configuration)

#define debug_compare(opt1, opt2)                       \
  check_enabled_shortcut(opt1, debug_local_options_on(opt2, none))

#define debug_cond_clause(condition, if_true, if_false) \
  meld_if_stmt(condition)(if_true)(if_false)
#define debug_cond_enabled(feature, if_true)                    \
  debug_cond_clause(debug_check_enabled_shortcut(feature),if_true,)
#define debug_cond_enabled_else(feature, if_true, if_false)             \
  debug_cond_clause(debug_check_enabled_shortcut(feature),if_true,if_false)
#define debug_cond(condition, if_true)                          \
  debug_cond_clause(condition,if_true,)
#define debug_cond_else(condition, if_true, if_false) \
  debug_cond_clause(condition,if_true,if_false)
#define debug_cond_force(condition, if_true) do { if_true; } while (0);
#define debug_test(feature, if_true, if_false)                  \
  meld_if_stmt(debug_check_enabled_shortcut(feature))(if_true)(if_false)

#define debug_cond_block(block, conds...)       \
  debug_cond_block_recur(block, conds)

#define debug_cond_block_recur(block, cond_fst, conds...)       \
  debug_cond_clause(cond_fst,                                   \
    meld_if_stmt(_meld_has_arguments(conds))(                   \
      debug_cond_block_recur(block, conds)                      \
    )(                                                          \
      block                                                     \
    )                                                           \
 ,)

/* External Interface Functions */
#define backend_check_enabled_options(test_option, options...)  \
  debug_check_enabled_shortcut_impl(test_option, options)
#define backend_check_enabled(test_option)                          \
  backend_check_enabled_options(test_option, backend_configuration)
#define backend_enable_if(feature, execute_if_true) \
  debug_cond_enabled(feature, execute_if_true)
#define backend_enable_if_else(feature, execute_if_true, execute_if_false) \
  debug_test(feature, execute_if_true, execute_if_false)
#define backend_enable_if_not(feature, execute_if_false)  \
  debug_test(feature, /*do nothing*/ , execute_if_false)
// #define backend_enable_lambda(feature)                      \
//   debug_test(feature, feature ## true  , feature ## false)

#endif  /*INCLUDED_FEATURES_ENABLE_IF*/
