
#if !defined INCLUDED_TPL_MELD_MAP
#define INCLUDED_TPL_MELD_MAP

#include "control.h"

#define _meld_arg_20(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, ...) _20
#define _meld_has_comma(arg...) _meld_arg_20(arg, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define _meld_trigger_parens_(...) ,
#define _meld_is_empty(arg...) _meld_is_empty_(                       \
    _meld_has_comma(arg),                                             \
    _meld_has_comma(_meld_trigger_parens_ arg),                       \
    _meld_has_comma(arg ()),                                          \
    _meld_has_comma(_meld_trigger_parens_ arg ())                     \
  )
#define _meld_paste_5(_0, _1, _2, _3, _4) _0 ## _1 ## _2 ## _3 ## _4
#define _meld_is_empty_(_0, _1, _2, _3) _meld_has_comma(        \
    _meld_paste_5(_meld_is_empty_case_, _0, _1, _2, _3)         \
  )
#define _meld_is_empty_case_0001 ,

#define _meld_has_arguments(arg...)                                     \
  _meld_to_bool(_meld_extract_first(_meld__argument_token_ arg)())
#define _meld__argument_token_() 0

#define _meld_stateless_map(fn, fst, arg...)               \
  fn(fst)                                                  \
  meld_if_stmt(_meld_has_arguments(arg))(                  \
    meld_defer_2(_meld__stateless_map)()(fn, arg)          \
  )()
#define _meld__stateless_map() _meld_stateless_map
#define _meld_map _meld_stateless_map

#define _meld_lookup_key(fn_keys_equal, key, out, cur_key, cur_val, arg...) \
  meld_if_stmt(fn_keys_equal(key,cur_key))(                             \
    cur_val                                                             \
  )(                                                                    \
    meld_if_stmt(_meld_has_arguments(arg))(                             \
      meld_defer_3(_meld__lookup_key)()(fn_keys_equal, key, out, arg)   \
    )(                                                                  \
      out                                                               \
    )                                                                   \
  )
#define _meld__lookup_key() _meld_lookup_key
#define _meld_lookup_key_wrap(fn_keys_equal, key, cur_key, cur_val, arg...)  \
  _meld_lookup_key(fn_keys_equal, key, #key, cur_key, cur_val, arg)
#define _meld_lookup_key_else(fn_keys_equal, key, out, cur_key, cur_val, arg...) \
  _meld_lookup_key(fn_keys_equal, key, out, cur_key, cur_val, arg)

#define _meld_print_kv(kv_printer, cur_key, cur_val, arg...)            \
  kv_printer(cur_key, cur_val)                                          \
  meld_if_stmt(_meld_has_arguments(arg))(                               \
    meld_defer_2(_meld__print_kv)()(kv_printer, arg)                    \
  )()
#define _meld__print_kv() _meld_print_kv
#define _meld_print_kv_wrap _meld_print_kv

#define _meld_check_enabled(check_fn, opt, cur_entry, arg_x...)         \
  meld_if_stmt(check_fn(opt, cur_entry))(1)                             \
  (                                                                     \
    meld_if_stmt(_meld_has_arguments(arg_x))(                           \
      meld_defer_3(_meld__check_enabled)()(check_fn, opt, arg_x)        \
    )(0)                                                                \
  )
#define _meld__check_enabled() _meld_check_enabled
#define _meld_check_enabled_wrap _meld_check_enabled

#define _meld_transform_key(\
  fn_keys_equal, key, fn_transform, cur_key, cur_val, arg...            \
)                                                                       \
  meld_if_stmt(fn_keys_equal(key,cur_key))(                             \
    fn_transform(cur_key,cur_val)                                       \
  )(                                                                    \
    meld_if_stmt(_meld_has_arguments(arg))(                             \
      meld_defer_3(_meld__transform_key)()(                             \
        fn_keys_equal, key, fn_transform, arg                           \
      )                                                                 \
    )()                                                                 \
  )
#define _meld__transform_key() _meld_transform_key
#define _meld_transform_key_wrap _meld_transform_key

#define _meld_stateless_map_with_(fn, with, fst, arg...)            \
  fn(fst, with)                                                     \
  meld_if_stmt(_meld_has_arguments(arg))(                           \
    meld_defer_2(_meld__stateless_map_with_)()(fn, with, arg)       \
  )()
#define _meld__stateless_map_with_() _meld_stateless_map_with_
#define _meld_map_with _meld_stateless_map_with_

#define _meld_map_lst(map_fn, lambda, lst) _meld_eval(map_fn(lambda,lst))

#define _meld_fold_or_(cur, fst, arg...)                               \
  meld_if_stmt(_meld_has_arguments(arg))(                              \
    meld_defer_2(_meld__fold_or_)()(_meld_bit_or(cur)(fst), arg)       \
  )(cur)
#define _meld__fold_or_() _meld_fold_or_
#define _meld_fold_or _meld_fold_or_
#define _meld_fold_or_helper(args...) meld_eval(_meld_fold_or(0, args))

// Interface for meld map
#define meld_is_empty _meld_is_empty
#define meld_map _meld_map
#define meld_map_with _meld_map_with
#define meld_meta_map _meld_map_lst
#define meld_fold_or _meld_fold_or_helper
#define meld_lookup_key _meld_lookup_key_wrap
#define meld_lookup_else _meld_lookup_key_else
#define meld_transform_key _meld_transform_key_wrap
#define meld_print_kv _meld_print_kv_wrap
#define meld_check_enabled  _meld_check_enabled_wrap

#endif
