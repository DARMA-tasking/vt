/*
//@HEADER
// *****************************************************************************
//
//                              context_functor.h
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

#if !defined INCLUDED_CONTEXT_SRC_CONTEXT_FUNCTOR_H
#define INCLUDED_CONTEXT_SRC_CONTEXT_FUNCTOR_H

#include "context/fcontext.h"
#include "context/context_wrapper.h"
#include "context/context_stack.h"

#include <functional>
#include <tuple>
#include <utility>

namespace fcontext { namespace functor {

template <typename FunctorT, typename TupleT>
struct ContextHolder {
  using FunctorTypeT = FunctorT;
  using TupleType = TupleT;

  TupleT tup;

  template <typename... Args>
  explicit ContextHolder(Args&&... a) : tup(std::forward<Args>(a)...) { }
};

template <typename FunctorT, typename TupleT>
struct ContextFunctor {
  using FunctorTypeT = FunctorT;
  using TupleType = TupleT;
  using ContextHolderPtrType = ContextHolder<FunctorT, TupleT>*;

  fcontext::Context ctx;
  ContextHolderPtrType holder = nullptr;

  ContextFunctor(Context const& in_ctx, ContextHolderPtrType in_holder)
    : ctx(in_ctx), holder(in_holder)
  { }

  ContextFunctor() = default;
};

template <typename Function, typename Tuple, size_t... I>
static auto call(
  Function f, fcontext_transfer_t transfer, Tuple&& t, std::index_sequence<I...>
) {
  return f(
    transfer,
    std::forward<typename std::tuple_element<I,Tuple>::type>(std::get<I>(t))...
  );
}

template <typename FnT, typename... Args>
static void dispatch(std::tuple<Args...>&& tup, FnT fn, fcontext_transfer_t t) {
  using TupleType = typename std::decay<decltype(tup)>::type;
  using TupleT = std::tuple<Args...>;
  static constexpr auto size = std::tuple_size<TupleType>::value;
  call(fn, t, std::forward<TupleT>(tup), std::make_index_sequence<size>{});
}

template <typename ContextHolderT>
static void context_functor_handler(fcontext_transfer_t t) {
  using TupleT = typename ContextHolderT::TupleType;
  ContextHolderT* ctx_holder = static_cast<ContextHolderT*>(t.data);
  typename ContextHolderT::FunctorTypeT fn;
  dispatch(std::forward<TupleT>(ctx_holder->tup), fn, t);
}

template <typename CtxFuncT>
auto jumpContextFunc(CtxFuncT ctx_func) {
  auto const& ult_ctx = ctx_func.ctx;
  return jump_fcontext(ult_ctx.ctx, static_cast<void*>(ctx_func.holder));
}

template <typename FunctorT, typename... Args>
ContextFunctor<FunctorT, std::tuple<Args...>> makeContext(ULTContext ctx, Args&&... a) {
  using TupleT = std::tuple<Args...>;
  using HolderType = ContextHolder<FunctorT, TupleT>;

  Context c = fcontext::makeContext(ctx, context_functor_handler<HolderType>);

  auto holder = new HolderType(std::forward<Args>(a)...);

  ContextFunctor<FunctorT, TupleT> ctx_functor(c, holder);

  return ctx_functor;
}

template <typename FunctorT, typename... Args>
using ContextF = ContextFunctor<FunctorT, std::tuple<Args...>>;

}} /* end namespace fcontext::functor */

#endif /*INCLUDED_CONTEXT_SRC_CONTEXT_FUNCTOR_H*/
