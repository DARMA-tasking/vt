/*
//@HEADER
// *****************************************************************************
//
//                                 param_meta.h
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

#if !defined INCLUDED_VT_PARAMETERIZATION_PARAM_META_H
#define INCLUDED_VT_PARAMETERIZATION_PARAM_META_H

#include "vt/config.h"

#include <tuple>
#include <utility>
#include <functional>
#include <type_traits>

namespace vt { namespace param {

template <typename... Args>
using MultiParamType = void(*)(Args...);

template<typename T, T value>
struct NonType {};

#define PARAM_FUNCTION_RHS(value) vt::param::NonType<decltype(&value),(&value)>()
#define PARAM_FUNCTION(value) decltype(&value),(&value)

template <typename Function, typename Tuple, size_t... I>
auto callFnTuple(Function f, Tuple t, std::index_sequence<I...>) {
  return f(
    std::forward<typename std::tuple_element<I,Tuple>::type>(
      std::get<I>(t)
    )...
  );
}

template <size_t size, typename TypedFnT, typename... Args>
void invokeFnTuple(TypedFnT f, std::tuple<Args...> t) {
  using TupleType = std::tuple<Args...>;
  callFnTuple(f, std::forward<TupleType>(t), std::make_index_sequence<size>{});
}

template <typename FnT, typename... Args>
void invokeCallableTuple(std::tuple<Args...>& tup, FnT fn, bool const& is_functor) {
  using TupleType = typename std::decay<decltype(tup)>::type;
  static constexpr auto size = std::tuple_size<TupleType>::value;
  if (is_functor) {
    auto typed_fn = reinterpret_cast<MultiParamType<Args...>>(fn);
    return invokeFnTuple<size>(typed_fn, tup);
  } else {
    auto typed_fn = reinterpret_cast<MultiParamType<Args...>>(fn);
    return invokeFnTuple<size>(typed_fn, tup);
  }
}

}} /* end namespace vt::param */

#endif /*INCLUDED_VT_PARAMETERIZATION_PARAM_META_H*/
