
#if ! defined __RUNTIME_TRANSPORT_PARAMETERIZATION__
#define __RUNTIME_TRANSPORT_PARAMETERIZATION__

#include "common.h"
#include "message.h"
#include "active.h"

namespace runtime { namespace param {

using handler_manager_t = runtime::HandlerManager;

template <typename... Args>
using multi_param_t = void(*)(Args...);

template<typename T, T value>
struct NonType {};

#define param_function_rhs(value) runtime::param::NonType<decltype(value),(value)>()
#define param_function(value) decltype(value),(value)

template <typename Tuple>
struct DataMsg : runtime::Message {
  Tuple tup;
  handler_t sub_han = uninitialized_handler;

  DataMsg(Tuple a, handler_t const& in_sub_han)
    : Message(), tup(a), sub_han(in_sub_han)
  { }
};

template <typename Function, typename Tuple, size_t... I>
auto call(Function f, Tuple t, std::index_sequence<I...>) {
  return f(std::get<I>(t)...);
}

template <typename FnT, typename... Args>
void get_fn_sig(std::tuple<Args...> tup, FnT fn) {
  static constexpr auto size = std::tuple_size<decltype(tup)>::value;
  auto typed_fn = reinterpret_cast<multi_param_t<Args...>>(fn);
  call(typed_fn, tup, std::make_index_sequence<size>{});
}

template <typename Tuple>
static void data_message_handler(DataMsg<Tuple>* msg) {
  auto fn = auto_registry::get_auto_handler(msg->sub_han);
  debug_print_param("data_message_handler: id=%d, fn=%p\n", msg->sub_han, fn);
  get_fn_sig(msg->tup, fn);
}

struct Param {
  template <typename T, T value, typename... Args>
  event_t send_data_helper(
    node_t const& dest, void (*fn)(Args...), std::tuple<Args...> tup,
    NonType<T, value>
  ) {
    auto const& han = auto_registry::make_auto_handler<T,value>();
    using tuple_type = decltype(tup);
    DataMsg<tuple_type>* m = new DataMsg<tuple_type>(tup, han);
    return the_msg->send_msg<DataMsg<tuple_type>, data_message_handler>(
      dest, m, [=]{ delete m; }
    );
  }

  template <typename T, T value, typename Tuple>
  event_t send_data(
    node_t const& dest, Tuple tup, NonType<T, value> non = NonType<T,value>()
  ) {
    return send_data_helper(dest, value, tup, non);
  }

  template <typename T, T value, typename... Args>
  event_t send_data(node_t const& dest, NonType<T, value> non, Args&&... a) {
    return send_data_helper(
      dest, value, std::make_tuple(std::forward<Args>(a)...), non
    );
  }

  template <typename T, T value, typename... Args>
  event_t send_data(node_t const& dest, Args&&... a) {
    return send_data_helper(
      dest, value, std::make_tuple(std::forward<Args>(a)...), NonType<T,value>()
    );
  }
};

}} //end namespace runtime::param

namespace runtime {

extern std::unique_ptr<param::Param> the_param;

template <typename... Args>
std::tuple<Args...> build_data(Args&&... a) {
  return std::tuple<Args...>(std::forward<Args>(a)...);
}

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_PARAMETERIZATION__*/
