/*
//@HEADER
// ************************************************************************
//
//                        meld_control.h
//                           darma
//              Copyright (C) 2016 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// JL => Jonathan Lifflander (jliffla@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#if ! defined __DARMA_CHARM_MELD_CONTROL__
#define __DARMA_CHARM_MELD_CONTROL__

/*
 * Control stmts for the meld interface
 */

#define _meld_extract_first(elm, ...) elm
#define _meld_extract_second(_, elm, ...) elm
#define _meld_is_probe(arg...) _meld_extract_second(arg, 0)
#define _meld_probe() ~, 1

#define _meld_concat_all(a, arg...) a ## arg
#define _meld_concat_all_(a, arg...) _meld_concat_all_(a, arg)
#define _meld_concat(a,b) a ## b

#define _meld_logical_not(x) _meld_is_probe(_meld_concat(_meld__logical_not_, x))
#define _meld__logical_not_0 _meld_probe()

#define _meld_to_bool(x) _meld_logical_not(_meld_logical_not(x))

#define _meld_if_else_clause(cond) _meld__if_else_clause(_meld_to_bool(cond))
#define _meld__if_else_clause(cond) _meld_concat(_meld__if_, cond)
#define _meld__if_1(arg...) arg _meld__if_1_else
#define _meld__if_0(arg...)     _meld__if_0_else
#define _meld__if_1_else(arg...)
#define _meld__if_0_else(arg...) arg

#define _meld_if_clause(cond, if_true_lambda)   \
  _meld_if_else_clause(cond)(if_true_lambda)()

#define _meld_complement(in) _meld_concat_all(_meld_complement_, in)
#define _meld_complement_0 1
#define _meld_complement_1 0

#define _meld_bit_and(x) _meld_concat_all(_meld_bit_and_, x)
#define _meld_bit_and_0(y) 0
#define _meld_bit_and_1(y) y

#define _meld_bit_or(x) _meld_concat_all(_meld_bit_or_, x)
#define _meld_bit_or_0(y) y
#define _meld_bit_or_1(y) 1

#define _meld_iif_stmt(c) _meld_concat_all(_meld_iff_stmt_, c)
#define _meld_iff_stmt_0(t, arg...) arg
#define _meld_iff_stmt_1(t, ...) t

// #define _meld_while_clause(pred, op, arg...)        \
//   _meld_if_clause(op(arg))(                         \
//     meld_defer_2(_meld_while_clause_)()(            \
//       pred, op, op(arg)                             \
//     ), arg                                          \
//   )
// #define _meld_while_clause_ _meld_while_clause

//meld_defer_2(_meld_switch_)()(cont)           \

// a swtich stmt that short circuits
#define _meld_switch_(condition, op, cont...) \
  meld_if_stmt(condition)(op)(               \
    meld_defer_2(_meld__switch_)()(cont)     \
  )
#define _meld__switch_() _meld_switch_

/*
 * if_stmt syntax:
 *   meld_if_stmt(condition)(if_true_stmt)(if_false_stmt)
 */

// Interface for meld combinators
#define meld_if_stmt _meld_if_else_clause
#define meld_if_stmt_only _meld_if_clause
#define meld_switch(stmts...) meld_eval(_meld_switch_(stmts))
#define meld_concat _meld_concat
#define meld_complement _meld_complement
#define meld_bit_and _meld_bit_and
#define meld_bit_or _meld_bit_or
#define meld_iif_stmt _meld_iif_stmt
#define meld_to_bool _meld_to_bool
#define meld_arg(arg...) meld_defer_1(meld_arg)()(arg)
#define meld_comma ,
#define meld_va_comma(arg...) , ##arg

/*
 * @todo implement some compiler specific functions to reveal macro state though
 * pragma messages for debugging meld
 */

// #define meld_stringize(x...) #x
// #define meld_value(x) meld_stringize(x)
// #define meld_name_value(var) #var "=" meld_value(var)
// #pragma message(meld_name_value(##debug_mode))

#endif
