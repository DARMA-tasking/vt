
#if ! defined __RUNTIME_TRANSPORT_PARAMETERIZATION__
#define __RUNTIME_TRANSPORT_PARAMETERIZATION__

#include "common.h"
#include "message.h"
#include "active.h"
#include "auto_registry_interface.h"

namespace vt { namespace param {

using HandlerManagerType = vt::HandlerManager;

template <typename... Args>
using MultiParamType = void(*)(Args...);

template<typename T, T value>
struct NonType {};

#define PARAM_FUNCTION_RHS(value) vt::param::NonType<decltype(value),(value)>()
#define PARAM_FUNCTION(value) decltype(value),(value)

template <typename Tuple>
struct DataMsg : vt::Message {
  Tuple tup;
  HandlerType sub_han = uninitialized_handler;

  DataMsg(HandlerType const& in_sub_han, Tuple&& a)
    : Message(), tup(std::forward<Tuple>(a)), sub_han(in_sub_han)
  { }

  template <typename... Args>
  DataMsg(HandlerType const& in_sub_han, Args&&... a)
    : Message(), tup(std::forward<Args>(a)...), sub_han(in_sub_han)
  { }
};

template <typename Function, typename Tuple, size_t... I>
static auto call(Function f, Tuple&& t, std::index_sequence<I...>) {
  return f(
    std::forward<typename std::tuple_element<I,Tuple>::type>(
      std::get<I>(t)
    )...
  );
}

template <typename FnT, typename... Args>
static void getFnSig(std::tuple<Args...>&& tup, FnT fn, bool const& is_functor) {
  using TupleType = typename std::decay<decltype(tup)>::type;
  static constexpr auto size = std::tuple_size<TupleType>::value;
  if (is_functor) {
    // be careful: functor version takes a r-value ref as `Args' and forwards
    auto typed_fn = reinterpret_cast<MultiParamType<Args&&...>>(fn);
    call(
      typed_fn, std::forward<std::tuple<Args...>>(tup),
      std::make_index_sequence<size>{}
    );
  } else {
    // be careful: non-fuctor version takes a l-value as `Args'
    auto typed_fn = reinterpret_cast<MultiParamType<Args...>>(fn);
    call(
      typed_fn, std::forward<std::tuple<Args...>>(tup),
      std::make_index_sequence<size>{}
    );
  }
}

template <typename Tuple>
static void dataMessageHandler(DataMsg<Tuple>* msg) {
  debug_print(
    param, node,
    "dataMessageHandler: id=%d\n", msg->sub_han
  );

#if backend_check_enabled(trace_enabled)
  trace::TraceEntryIDType ep = auto_registry::get_trace_id(msg->sub_han);
  trace::TraceEventIDType event = envelope_get_trace_event(msg->env);

  printf("dataMessageHandler: id=%d, ep=%lu\n", msg->sub_han, ep);

  NodeType const& from_node = theMsg->getFromNodeCurrentHandler();

  theTrace->begin_processing(ep, sizeof(*msg), event, from_node);
#endif

  if (HandlerManagerType::isHandlerFunctor(msg->sub_han)) {
    auto fn = auto_registry::getAutoHandlerFunctor(msg->sub_han);
    getFnSig(std::forward<Tuple>(msg->tup), fn, true);
  } else {
    // regular active function
    auto fn = auto_registry::getAutoHandler(msg->sub_han);
    getFnSig(std::forward<Tuple>(msg->tup), fn, false);
  }

#if backend_check_enabled(trace_enabled)
  theTrace->end_processing(ep, sizeof(*msg), event, from_node);
#endif
}

template <bool...> struct BoolPack;
template <bool... bs>
using all_true = std::is_same<BoolPack<bs..., true>, BoolPack<true, bs...>>;

struct Param {

  template <typename... Args>
  EventType sendDataTuple(
    NodeType const& dest, HandlerType const& han, std::tuple<Args...>&& tup
  ) {
    staticCheckCopyable<Args...>();

    using TupleType = typename std::decay<decltype(tup)>::type;

    DataMsg<TupleType>* m = new DataMsg<TupleType>(
      han, std::forward<std::tuple<Args...>>(tup)
    );

    return theMsg->sendMsg<DataMsg<TupleType>, dataMessageHandler>(
      dest, m, [=]{ delete m; }
    );
  }

  template <typename... Args>
  void staticCheckCopyable() {
    using cond = all_true<std::is_trivially_copyable<Args>::value...>;

    static_assert(
      std::is_same<typename cond::type,std::true_type>::value == true,
      "All types passed for parameterization must be trivially copyable"
    );
  }

  template <typename DataMsg>
  EventType sendDataMsg(NodeType const& dest, HandlerType const& han, DataMsg* m) {
    return theMsg->sendMsg<DataMsg, dataMessageHandler>(
      dest, m, [=]{ delete m; }
    );
  }

  template <typename T, T value, typename Tuple>
  EventType sendData(
    NodeType const& dest, Tuple tup, NonType<T, value> non = NonType<T,value>()
  ) {
    auto const& han = auto_registry::makeAutoHandler<T,value>();
    return sendDataTuple(dest, han, std::forward<Tuple>(tup));
  }

  template <typename T, T value, typename... Args>
  EventType sendData(
    NodeType const& dest, DataMsg<std::tuple<Args...>>* msg,
    NonType<T, value> non = NonType<T,value>()
  ) {
    auto const& han = auto_registry::makeAutoHandler<T,value>();
    msg->sub_han = han;
    return sendDataMsg(dest, han, msg);
  }

  template <typename T, T value, typename... Args>
  EventType sendData(NodeType const& dest, NonType<T, value> non, Args&&... a) {
    auto const& han = auto_registry::makeAutoHandler<T,value>();

    staticCheckCopyable<Args...>();

    using TupleType = std::tuple<Args...>;

    DataMsg<TupleType>* m = new DataMsg<TupleType>(
      han, std::forward<Args>(a)...
    );

    return sendDataMsg(dest, han, m);
  }

  template <typename T, T value, typename... Args>
  EventType sendData(NodeType const& dest, Args&&... a) {
    return sendData(dest, NonType<T,value>(), std::forward<Args>(a)...);
  }

  /*
   * Functor variants
   */

  template <typename FunctorT, typename... Args>
  EventType sendDataHelperFunctor(
    NodeType const& dest, std::tuple<Args...>&& tup
  ) {
    using MessageType = vt::BaseMessage;
    auto const& han = auto_registry::makeAutoHandlerFunctor<
      FunctorT, false, Args...
    >();
    return sendDataTuple(dest, han, std::forward<std::tuple<Args...>>(tup));
  }

  template <typename FunctorT, typename Tuple>
  EventType sendData(NodeType const& dest, Tuple tup) {
    return sendDataHelperFunctor<FunctorT>(dest, std::forward<Tuple>(tup));
  }

  template <typename FunctorT, typename... Args>
  EventType sendData(NodeType const& dest, DataMsg<std::tuple<Args...>>* msg) {
    staticCheckCopyable<Args...>();

    auto const& han = auto_registry::makeAutoHandlerFunctor<
      FunctorT, false, Args...
    >();
    msg->sub_han = han;

    return sendDataMsg(dest, han, msg);
  }

  template <typename FunctorT, typename... Args>
  EventType sendData(NodeType const& dest, Args&&... a) {
    staticCheckCopyable<Args...>();

    auto const& han = auto_registry::makeAutoHandlerFunctor<
      FunctorT, false, Args...
    >();

    using TupleType = std::tuple<Args...>;

    DataMsg<TupleType>* m = new DataMsg<TupleType>(
      han, std::forward<Args>(a)...
    );

    return sendDataMsg(dest, han, m);
  }
};

}} //end namespace vt::param

namespace vt {

extern std::unique_ptr<param::Param> theParam;

template <typename... Args>
param::DataMsg<std::tuple<Args...>>* buildData(Args&&... a) {
  return new param::DataMsg<std::tuple<Args...>>(
    uninitialized_handler, std::forward<Args>(a)...
  );
}

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_PARAMETERIZATION__*/
