
#if !defined __RUNTIME_TRANSPORT_PARAM_META__
#define __RUNTIME_TRANSPORT_PARAM_META__

#include "config.h"

namespace vt { namespace param {

template <typename... Args>
using MultiParamType = void(*)(Args...);

template<typename T, T value>
struct NonType {};

#define PARAM_FUNCTION_RHS(value) vt::param::NonType<decltype(value),(value)>()
#define PARAM_FUNCTION(value) decltype(value),(value)

template <typename Function, typename Tuple, size_t... I>
auto callFnTuple(Function f, Tuple&& t, std::index_sequence<I...>) {
  return f(
    std::forward<typename std::tuple_element<I,Tuple>::type>(
      std::get<I>(t)
    )...
  );
}

template <size_t size, typename TypedFnT, typename... Args>
void invokeFnTuple(TypedFnT f, std::tuple<Args...>&& t) {
  using TupleType = std::tuple<Args...>;
  callFnTuple(f, std::forward<TupleType>(t), std::make_index_sequence<size>{});
}

template <typename FnT, typename... Args>
void invokeCallableTuple(std::tuple<Args...>&& tup, FnT fn, bool const& is_functor) {
  using TupleType = typename std::decay<decltype(tup)>::type;
  static constexpr auto size = std::tuple_size<TupleType>::value;
  if (is_functor) {
    auto typed_fn = reinterpret_cast<MultiParamType<Args&&...>>(fn);
    return invokeFnTuple<size>(typed_fn, std::forward<TupleType>(tup));
  } else {
    auto typed_fn = reinterpret_cast<MultiParamType<Args...>>(fn);
    return invokeFnTuple<size>(typed_fn, std::forward<TupleType>(tup));
  }
}

template <bool...> struct BoolPack;
template <bool... bs>
using all_true = std::is_same<BoolPack<bs..., true>, BoolPack<true, bs...>>;

}} /* end namespace vt::param */

#endif /*__RUNTIME_TRANSPORT_PARAM_META__*/
