
#if !defined INCLUDED_TPL_MELD_COMBINATORS
#define INCLUDED_TPL_MELD_COMBINATORS

#define _meld_eval(arg...) _meld_eval_1024(arg)
#define _meld_eval_65536(arg...) _meld_eval_32768(_meld_eval_32768(arg))
#define _meld_eval_32768(arg...) _meld_eval_16384(_meld_eval_16384(arg))
#define _meld_eval_16384(arg...) _meld_eval_8192(_meld_eval_8192(arg))
#define _meld_eval_8192(arg...) _meld_eval_4096(_meld_eval_4096(arg))
#define _meld_eval_4096(arg...) _meld_eval_2048(_meld_eval_2048(arg))
#define _meld_eval_2048(arg...) _meld_eval_1024(_meld_eval_1024(arg))
#define _meld_eval_1024(arg...) _meld_eval_512(_meld_eval_512(arg))
#define _meld_eval_512(arg...) _meld_eval_256(_meld_eval_256(arg))
#define _meld_eval_256(arg...) _meld_eval_128(_meld_eval_128(arg))
#define _meld_eval_128(arg...) _meld_eval_64(_meld_eval_64(arg))
#define _meld_eval_64(arg...) _meld_eval_32(_meld_eval_32(arg))
#define _meld_eval_32(arg...) _meld_eval_16(_meld_eval_16(arg))
#define _meld_eval_16(arg...) _meld_eval_8(_meld_eval_8(arg))
#define _meld_eval_8(arg...) _meld_eval_4(_meld_eval_4(arg))
#define _meld_eval_4(arg...) _meld_eval_2(_meld_eval_2(arg))
#define _meld_eval_2(arg...) _meld_eval_1(_meld_eval_1(arg))
#define _meld_eval_1(arg...) arg

#define _meld_defer_thunk()
#define _meld_defer_1(fn) fn _meld_defer_thunk()
#define _meld_defer_2(fn) fn _meld_defer_thunk _meld_defer_thunk()()
#define _meld_defer_3(fn) fn _meld_defer_thunk _meld_defer_thunk _meld_defer_thunk()()()
#define _meld_defer_4(fn) fn _meld_defer_thunk _meld_defer_thunk _meld_defer_thunk _meld_defer_thunk()()()()

// Interface for meld combinators
#define meld_eval _meld_eval_16
#define meld_eval_2 _meld_eval_2
#define meld_eval_4 _meld_eval_4
#define meld_eval_8 _meld_eval_8
#define meld_eval_16 _meld_eval_16
#define meld_eval_32 _meld_eval_32
#define meld_eval_64 _meld_eval_64
#define meld_eval_128 _meld_eval_128

#define meld_defer_1 _meld_defer_1
#define meld_defer_2 _meld_defer_2
#define meld_defer_3 _meld_defer_3
#define meld_defer_4 _meld_defer_4

#endif  /*INCLUDED_TPL_MELD_COMBINATORS*/
