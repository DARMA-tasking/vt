
#if !defined INCLUDED_PIPE_PIPE_MANAGER_CONSTRUCT_H
#define INCLUDED_PIPE_PIPE_MANAGER_CONSTRUCT_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/pipe_manager.h"
#include "pipe/state/pipe_state.h"
#include "pipe/interface/remote_container_msg.h"
#include "pipe/interface/send_container.h"
#include "pipe/interface/callback_direct.h"
#include "pipe/id/pipe_id.h"
#include "context/context.h"
#include "registry/auto/auto_registry.h"

#include <cassert>
#include <tuple>
#include <type_traits>
#include <array>

namespace vt { namespace pipe {

template <std::size_t N, typename ValueT>
struct RepeatNImpl {
  using ResultType = decltype(
    std::tuple_cat(
      std::declval<std::tuple<ValueT>>(),
      std::declval<typename RepeatNImpl<N-1,ValueT>::ResultType>()
  ));
};

template <typename ValueT>
struct RepeatNImpl<0,ValueT> {
  using ResultType = std::tuple<>;
};

template <
  std::size_t cur, typename T, typename ConsT, typename U, std::size_t N,
  ActiveTypedFnType<U>*... f
>
struct ConstructCallbacksImpl;

template <
  std::size_t cur, typename T, typename ConsT, typename U, std::size_t N,
  ActiveTypedFnType<U>* fcur, ActiveTypedFnType<U>*... f
>
struct ConstructCallbacksImpl<cur,T,ConsT,U,N,fcur,f...> {
  using ResultType = decltype(
    std::tuple_cat(
      std::declval<std::tuple<T>>(),
      std::declval<
        typename ConstructCallbacksImpl<cur+1,T,ConsT,U,N,f...>::ResultType
      >()
  ));
  static ResultType make(ConsT const& ct);
};

template <typename T, typename Tuple, size_t... I>
static T&& build(HandlerType han, Tuple t, std::index_sequence<I...>) {
  return std::move(T{
    han,
    std::forward<typename std::tuple_element<I,Tuple>::type>(
      std::get<I>(t)
    )...
  });
}

template <std::size_t size, typename T, typename... Args>
static T&& build(HandlerType han, std::tuple<Args...> t) {
  using TupleType = std::tuple<Args...>;
  return std::move(build<T>(han, t, std::make_index_sequence<size>{}));
}

template <
  std::size_t cur, typename T, typename ConsT, typename U, std::size_t N,
  ActiveTypedFnType<U>* fcur, ActiveTypedFnType<U>*... f
>
/*static*/ typename ConstructCallbacksImpl<cur,T,ConsT,U,N,fcur,f...>::ResultType
ConstructCallbacksImpl<cur,T,ConsT,U,N,fcur,f...>::make(ConsT const& ct)  {
  static constexpr auto size = std::tuple_size<
    typename std::tuple_element<cur,ConsT>::type
  >::value;
  ::fmt::print("val={}\n",std::get<0>(std::get<cur>(ct)));
  return std::tuple_cat(
    std::make_tuple(
      build<size,T>(
        auto_registry::makeAutoHandler<U,fcur>(nullptr), std::get<cur>(ct)
      )
    ),
    ConstructCallbacksImpl<cur+1,T,ConsT,U,N,f...>::make(ct)
  );
}

template <typename T, typename ConsT, typename U, std::size_t N>
struct ConstructCallbacksImpl<N,T,ConsT,U,N> {
  using ResultType = std::tuple<>;
  static ResultType make(ConsT const& ct);
};

template <typename T, typename ConsT, typename U, std::size_t N>
/*static*/ typename ConstructCallbacksImpl<N,T,ConsT,U,N>::ResultType
ConstructCallbacksImpl<N,T,ConsT,U,N>::make(ConsT const& ct) {
  return {};
}

template <typename T, typename ConsT, typename U, ActiveTypedFnType<U>*... f>
struct ConstructCallbacks {
  using Parent = ConstructCallbacksImpl<0,T,ConsT,U,sizeof...(f),f...>;
  using ResultType = typename Parent::ResultType;
  static ResultType make(ConsT const& ct);
};

template <typename T, typename ConsT, typename U, ActiveTypedFnType<U>*... f>
/*static*/ typename ConstructCallbacks<T,ConsT,U,f...>::ResultType
ConstructCallbacks<T,ConsT,U,f...>::make(ConsT const& c) {
  return ConstructCallbacks<T,ConsT,U,f...>::Parent::make(c);
}

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_CONSTRUCT_H*/
