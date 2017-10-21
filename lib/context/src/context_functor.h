
#if !defined INCLUDED_CONTEXT_SRC_CONTEXT_FUNCTOR_H
#define INCLUDED_CONTEXT_SRC_CONTEXT_FUNCTOR_H

#include "fcontext.h"
#include "context_wrapper.h"
#include "stack.h"

#include <functional>
#include <tuple>

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
using Context = ContextFunctor<FunctorT, std::tuple<Args...>>;

}} /* end namespace fcontext::functor */

#endif /*INCLUDED_CONTEXT_SRC_CONTEXT_FUNCTOR_H*/
