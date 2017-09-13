
#if !defined INCLUDED_VTEM_COMPARE
#define INCLUDED_VTEM_COMPARE

#include "vtem_control.h"

#define _meld_check_n(x, n, ...) n
#define _meld_check(arg...) _meld_check_n(arg, 0,)
#define _meld_check_probe(x) x, 1,

#define _meld_is_paren(x) _meld_check(_meld_is_paren_probe x)
#define _meld_is_paren_probe(...) _meld_check_probe(~)

#define _meld_comparable(x) _meld_is_paren(_meld_concat(debug_, x) (()))

#define _meld_token_compare(a, b) _meld_token_compare_(a, b)
#define _meld_token_compare_(a, b)                                      \
  meld_iif_stmt(                                                        \
    _meld_bit_and(_meld_comparable(a))(_meld_comparable(b))             \
  )(                                                                    \
    _meld_complement(                                                   \
      _meld_is_paren(                                                   \
        _meld_concat(debug_,a)(_meld_concat(debug_,b))(()))), 0         \
  )                                                                     \

// Interface for meld compare
#define meld_token_compare _meld_token_compare

#endif
