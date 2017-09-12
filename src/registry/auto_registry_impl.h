#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_IMPL__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_IMPL__

#include "common.h"
#include "auto_registry_common.h"
#include "auto_registry.h"

#include <vector>
#include <memory>

namespace runtime { namespace auto_registry {

template <typename>
inline auto_active_container_t&
get_auto_registry()  {
  static auto_active_container_t reg;
  return reg;
}

template <typename MessageT, action_any_function_t<MessageT>* f>
inline HandlerType
make_auto_handler(MessageT* const msg) {
  HandlerType const id = get_handler_active_function_expand(
    action_any_function_t<MessageT>, f
  );
  return handler_manager_t::make_handler(true, false, id);
}

template <typename T, T value>
inline HandlerType
make_auto_handler() {
  HandlerType const id = get_handler_active_function_expand(T, value);
  return handler_manager_t::make_handler(true, false, id);
}

template <typename ActiveFnT>
Registrar<ActiveFnT>::Registrar() {
  auto_active_container_t& reg = get_auto_registry<>();
  index = reg.size();
  auto fn = ActiveFnT::get_function();

  #if backend_check_enabled(trace_enabled)
  auto const& name = trace::DemanglerUtils::get_demangled_type<ActiveFnT>();
  auto const& parsed_names =
    trace::ActiveFunctionDemangler::parse_active_function_name(name);
  auto const& namespace_name = std::get<0>(parsed_names);
  auto const& function_name = std::get<1>(parsed_names);
  auto const& trace_ep = trace::TraceRegistry::register_event_hashed(
    namespace_name, function_name
  );

  reg.emplace_back(auto_reg_info_t<auto_active_t>{
    reinterpret_cast<active_basic_function_t*>(fn), trace_ep
  });
  #else
  reg.emplace_back(auto_reg_info_t<auto_active_t>{
    reinterpret_cast<active_basic_function_t*>(fn)
  });
  #endif
}

inline auto_active_t
get_auto_handler(HandlerType const& handler) {
  auto const& han_id = handler_manager_t::get_handler_identifier(handler);

  bool const& is_auto = handler_manager_t::is_handler_auto(handler);
  bool const& is_functor = handler_manager_t::is_handler_functor(handler);

  debug_print(
    handler, node,
    "get_auto_handler: handler=%d, id=%d, is_auto=%s, is_functor=%s\n",
    handler, han_id, print_bool(is_auto), print_bool(is_functor)
  );

  assert(
    not is_functor and is_auto and "Handler should not be a functor, but auto"
  );

  return get_auto_registry().at(han_id).get_fun();
}

template <typename ActiveFnT>
auto_HandlerType
register_active_fn() {
  return RegistrarWrapper<ActiveFnT>().registrar.index;
}

template <typename Callable>
/*static*/ constexpr typename Runnable<Callable>::function_ptr_type*
Runnable<Callable>::get_function() {
  return Callable::get_function();
}

}} // end namespace runtime::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_IMPL__*/
