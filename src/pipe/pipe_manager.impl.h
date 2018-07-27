
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

namespace vt { namespace pipe {

template <typename MsgT>
void PipeManager::triggerSendBack(PipeType const& pipe, MsgT* data) {
  auto const& this_node = theContext()->getNode();
  auto const& node_back = PipeIDBuilder::getNode(pipe);
  if (node_back != this_node) {
    // Send the message back to the owner node
    assert(0);
  } else {
    // Directly trigger the action because the pipe meta-data is located here
    assert(0);
  }
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
PipeManager::CallbackSendType<MsgT>
PipeManager::makeCallbackSingleSendTyped(
  bool const is_persist, NodeType const& send_to_node
) {
  // using SendContainerType = typename interface::SendContainer<MsgT>;
  auto const& new_pipe_id = makePipeID(is_persist,false);
  auto const& handler = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  auto container = CallbackSendType<MsgT>(
    new_pipe_id,send_to_node,handler
  );
  return container;
}


// template <typename MsgT, ActiveTypedFnType<MsgT>*... Args>
// struct HandlerHolder {
//   static constexpr ActiveTypedFnType<MsgT>* data[sizeof...(Args)] = {Args...};
// };


// template <typename T, typename ValueT, ValueT... Vals>
// struct GenerateTypePackImpl;

// template <typename T, typename ValueT, ValueT Val, ValueT... Vals>
// struct GenerateTypePackImpl<T,ValueT,Val,Vals...> {
//   using ResultType = decltype(
//     std::tuple_cat(
//       std::declval<std::tuple<T>>(),
//       std::declval<typename GenerateTypePackImpl<T,ValueT,Vals...>::ResultType>()
//   ));
//   static ResultType value;
// };

// template <typename T, typename ValueT, ValueT Val, ValueT... Vals>
// /*static*/ typename GenerateTypePackImpl<T,ValueT,Val,Vals...>::ResultType
// GenerateTypePackImpl<T,ValueT,Val,Vals...>::value =  std::tuple_cat(
//   std::make_tuple(T{Val}),
//   GenerateTypePackImpl<T,ValueT,Vals...>::value
// );

// template <typename T, typename ValueT>
// struct GenerateTypePackImpl<T,ValueT> {
//   using ResultType = std::tuple<>;
//   static ResultType value;
// };

// template <typename T, typename ValueT>
// /*static*/ typename GenerateTypePackImpl<T,ValueT>::ResultType
// GenerateTypePackImpl<T,ValueT>::value = {};

// template <typename T, typename ValueT, ValueT... Vals>
// struct GenerateTypePack {
//   using Parent = GenerateTypePackImpl<T,ValueT,Vals...>;
//   using ResultType = typename Parent::ResultType;
//   static ResultType value;
// };

// template <typename T, typename ValueT, ValueT... Vals>
// /*static*/ typename GenerateTypePack<T,ValueT,Vals...>::ResultType
// GenerateTypePack<T,ValueT,Vals...>::value =
// GenerateTypePack<T,ValueT,Vals...>::Parent::value;




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





// template <typename T, ActiveTypedFnType<T>*... Args>
// struct HandlerRegHolder {
//   static constexpr ActiveTypedFnType<T>* handlers[sizeof...(Args)] = { Args... };
// };


// template <typename T, size_t N, ActiveTypedFnType<T>*... f>
// struct HandlePackRegImpl {
//   using ResultType = decltype(
//     std::tuple_cat(
//       std::declval<std::tuple<T>>(),
//       std::declval<typename HandlePackRegImpl<T,N-1>::ResultType>()
//   ));
// };




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
  // static ResultType tuple;
  static ResultType make(ConsT const& ct);
};

// template <
//   std::size_t cur, typename T, typename ConsT, typename U, std::size_t N,
//   ActiveTypedFnType<U>* fcur, ActiveTypedFnType<U>*... f
// >
// /*static*/ typename ConstructCallbacksImpl<cur,T,ConsT,U,N,fcur,f...>::ResultType
// ConstructCallbacksImpl<cur,T,ConsT,U,N,fcur,f...>::tuple =
//   std::tuple_cat(
//     std::make_tuple(T{auto_registry::makeAutoHandler<U,fcur>(nullptr),0}),
//     ConstructCallbacksImpl<cur+1,T,ConsT,U,N,f...>::tuple
//   );


template <typename T, typename Tuple, size_t... I>
T&& build(HandlerType han, Tuple t, std::index_sequence<I...>) {
  return std::move(T{
    han,
    std::forward<typename std::tuple_element<I,Tuple>::type>(
      std::get<I>(t)
    )...
  });
}

template <std::size_t size, typename T, typename... Args>
T&& build(HandlerType han, std::tuple<Args...> t) {
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
  // static ResultType tuple;
  static ResultType make(ConsT const& ct);
};

// template <typename T, typename ConsT, typename U, std::size_t N>
// /*static*/ typename ConstructCallbacksImpl<N,T,ConsT,U,N>::ResultType
// ConstructCallbacksImpl<N,T,ConsT,U,N>::tuple = typename
//   ConstructCallbacksImpl<N,T,ConsT,U,N>::ResultType{};

template <typename T, typename ConsT, typename U, std::size_t N>
/*static*/ typename ConstructCallbacksImpl<N,T,ConsT,U,N>::ResultType
ConstructCallbacksImpl<N,T,ConsT,U,N>::make(ConsT const& ct) {
  return {};
}


template <typename T, typename ConsT, typename U, ActiveTypedFnType<U>*... f>
struct ConstructCallbacks {
  using Parent = ConstructCallbacksImpl<0,T,ConsT,U,sizeof...(f),f...>;
  using ResultType = typename Parent::ResultType;
  // static ResultType tuple;
  static ResultType make(ConsT const& ct);
};

// template <typename T, typename ConsT, typename U, ActiveTypedFnType<U>*... f>
// /*static*/ typename ConstructCallbacks<T,ConsT,U,f...>::ResultType
// ConstructCallbacks<T,ConsT,U,f...>::tuple =
// ConstructCallbacks<T,ConsT,U,f...>::Parent::tuple;

template <typename T, typename ConsT, typename U, ActiveTypedFnType<U>*... f>
/*static*/ typename ConstructCallbacks<T,ConsT,U,f...>::ResultType
ConstructCallbacks<T,ConsT,U,f...>::make(ConsT const& c) {
  return ConstructCallbacks<T,ConsT,U,f...>::Parent::make(c);
}

template <typename MsgT, ActiveTypedFnType<MsgT>*... f>
auto
PipeManager::makeCallbackMultiSendTyped(
  bool const is_persist, NodeType const& send_to_node
) {
  auto const& new_pipe_id = makePipeID(is_persist,false);
  using CBSendT = callback::CallbackSend<MsgT>;
  using ConsT = std::tuple<NodeType>;
  using TupleConsT = typename RepeatNImpl<sizeof...(f),ConsT>::ResultType;
  std::array<NodeType,sizeof...(f)> send_node_array;
  send_node_array.fill(send_to_node);

  ::fmt::print("arr val0={} val1={}\n", send_node_array[0], send_node_array[1]);
  auto const cons = TupleConsT{send_node_array};
  ::fmt::print("tup val0={} val1={}\n", std::get<0>(std::get<0>(cons)), std::get<0>(std::get<1>(cons)));
  using ConstructMeta = ConstructCallbacks<CBSendT,TupleConsT,MsgT,f...>;
  using TupleCBType = typename ConstructMeta::ResultType;
  auto const tuple = ConstructMeta::make(cons);
  auto rcm = interface::CallbackDirectSendMulti<MsgT,TupleCBType>(
    interface::CallbackDirectSendMultiTag,new_pipe_id,tuple
  );
  return rcm;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
void PipeManager::makeCallbackSingleBcast(bool const is_persist) {
  auto const& new_pipe_id = makePipeID(is_persist,false);
}

}} /* end namespace vt::pipe */
