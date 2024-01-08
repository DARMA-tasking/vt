/*
//@HEADER
// *****************************************************************************
//
//                              tuple_op_helper.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_TUPLE_OP_HELPER_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_TUPLE_OP_HELPER_H

#include <tuple>

namespace vt { namespace collective { namespace reduce { namespace operators {

template <typename Op, int cur, int max, typename enabled_ = void>
struct ApplyOp;

template <typename Op, int cur, int max>
struct ApplyOp<Op, cur, max, std::enable_if_t<cur != max>> {
  template <typename Tuple1, typename Tuple2>
  static void apply(Tuple1& t1, Tuple2 const& t2) {
    using CurType = std::decay_t<std::tuple_element_t<cur, Tuple1>>;
    using OpType = typename Op::template GetAsType<CurType>;
    OpType()(std::get<cur>(t1),std::get<cur>(t2));
    ApplyOp<Op, cur+1, max>::apply(t1, t2);
  }
};

template <typename Op, int cur, int max>
struct ApplyOp<Op, cur, max, std::enable_if_t<cur == max>> {
  template <typename Tuple1, typename Tuple2>
  static void apply(Tuple1& t1, Tuple2 const& t2) { }
};

//
// This is the cleaner way that is rejected by NVCC and Intel
//
// template <
//   template <typename X> class Op,
//   typename... Ts, typename... Us, std::size_t... I
// >
// void opTuple(
//   std::tuple<Ts...>& t1, std::tuple<Us...> const& t2, std::index_sequence<I...>
// ) {
//   std::forward_as_tuple(
//     (Op<std::decay_t<decltype(std::get<I>(t1))>>()(std::get<I>(t1),std::get<I>(t2)),0)...
//   );
// }

template <typename Op, typename Tuple1, typename Tuple2>
void opTuple(Tuple1& t1, Tuple2 const& t2) {
  // opTuple<Op>(t1, t2, std::make_index_sequence<sizeof...(Ts)>{});
  ApplyOp<Op, 0, std::tuple_size<Tuple1>{}>::apply(t1, t2);
}

}}}} /* end namespace vt::collective::reduce::operators */

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_TUPLE_OP_HELPER_H*/
