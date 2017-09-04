#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR_IMPL__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR_IMPL__

#include "common.h"
#include "auto_registry_common.h"
#include "auto_registry_functor.h"

#include <vector>
#include <memory>

namespace runtime { namespace auto_registry {

template <typename>
inline auto_active_functor_container_t& get_auto_registry_functor()  {
  static auto_active_functor_container_t reg;
  return reg;
}

template <typename FunctorT, bool is_msg, typename... Args>
inline handler_t make_auto_handler_functor() {
  handler_t const id = get_handler_active_functor(FunctorT, is_msg, Args);
  return handler_manager_t::make_handler(true, true, id);
}

template <typename RunnableFunctorT, typename... Args>
static inline void functor_handler_wrapper_rval(Args&&... args) {
  typename RunnableFunctorT::functor_t instance;
  return instance.operator ()(std::forward<Args>(args)...);
}

template <typename RunnableFunctorT, typename... Args>
static inline void functor_handler_wrapper_reg(Args... args) {
  typename RunnableFunctorT::functor_t instance;
  return instance.operator ()(args...);
}

template <typename RunnableFunctorT, typename... Args>
static inline void pull_apart(
  auto_active_functor_container_t& reg, bool const& is_msg,
  pack<Args...> packed_args
) {
  #if backend_check_enabled(trace_enabled)
  auto const& name = trace::DemanglerUtils::get_demangled_type<RunnableFunctorT>();
  auto const& parsed_names =
    trace::ActiveFunctorDemangler::parse_active_functor_name(name);
  auto const& namespace_name = std::get<0>(parsed_names);
  auto const& function_name = std::get<1>(parsed_names);
  auto const& trace_ep = trace::TraceRegistry::register_event_hashed(
    namespace_name, function_name
  );
  #endif

  if (is_msg) {
    auto fn_ptr = functor_handler_wrapper_reg<RunnableFunctorT, Args...>;
    reg.emplace_back(auto_reg_info_t<auto_active_functor_t>{
      reinterpret_cast<simple_function_t>(fn_ptr)
        #if backend_check_enabled(trace_enabled)
        , trace_ep
        #endif
    });
  } else {
    auto fn_ptr = functor_handler_wrapper_rval<RunnableFunctorT, Args...>;
    reg.emplace_back(auto_reg_info_t<auto_active_functor_t>{
      reinterpret_cast<simple_function_t>(fn_ptr)
        #if backend_check_enabled(trace_enabled)
        , trace_ep
        #endif
    });
  }
}

template <typename RunnableFunctorT>
RegistrarFunctor<RunnableFunctorT>::RegistrarFunctor() {
  auto_active_functor_container_t& reg = get_auto_registry_functor<>();
  index = reg.size();

  pull_apart<RunnableFunctorT>(
    reg, RunnableFunctorT::is_msg_t, typename RunnableFunctorT::packed_args_t()
  );
}

inline auto_active_functor_t get_auto_handler_functor(handler_t const& handler) {
  auto const& han_id = handler_manager_t::get_handler_identifier(handler);

  bool const& is_auto = handler_manager_t::is_handler_auto(handler);
  bool const& is_functor = handler_manager_t::is_handler_functor(handler);

  debug_print(
    handler, node,
    "get_auto_handler: handler=%d, id=%d, is_auto=%s, is_functor=%s\n",
    handler, han_id, print_bool(is_auto), print_bool(is_functor)
  );

  assert(
    is_functor and is_auto and "Handler should be a auto functor"
  );

  return get_auto_registry_functor().at(han_id).get_fun();
}

template <typename RunnableFunctorT>
auto_handler_t register_active_functor() {
  return RegistrarWrapperFunctor<RunnableFunctorT>().registrar.index;
}

}} // end namespace runtime::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR_IMPL__*/
