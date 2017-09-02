#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR_IMPL__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR_IMPL__

#include "common.h"
#include "auto_registry_common.h"
#include "auto_registry_functor.h"

#include <vector>
#include <memory>

namespace runtime { namespace auto_registry {

template <typename>
inline auto_active_functor_container_t&
get_auto_registry_functor()  {
  static auto_active_functor_container_t reg;
  return reg;
}

template <typename T>
inline handler_t
make_auto_handler_functor() {
  handler_t const id = get_handler_active_functor(T);
  return handler_manager_t::make_handler(true, true, id);
}

template <typename RunnableFunctorT>
RegistrarFunctor<RunnableFunctorT>::RegistrarFunctor() {
  auto_active_functor_container_t& reg = get_auto_registry_functor<>();
  index = reg.size();
  typename RunnableFunctorT::functor_t instance;
  reg.emplace_back(instance);
}

inline auto_active_functor_t
get_auto_handler_functor(handler_t const& handler) {
  auto const& han_id = handler_manager_t::get_handler_identifier(handler);

  bool const& is_auto = handler_manager_t::is_handler_auto(handler);
  bool const& is_functor = handler_manager_t::is_handler_functor(handler);

  debug_print_handler(
    "get_auto_handler: handler=%d, id=%d, is_auto=%s, is_functor=%s\n",
    handler, han_id, print_bool(is_auto), print_bool(is_functor)
  );

  assert(
    is_functor and is_auto and "Handler should be a auto functor"
  );

  return get_auto_registry_functor().at(han_id);
}

template <typename FunctorT>
auto_handler_t
register_active_functor() {
  return RegistrarWrapperFunctor<FunctorT>().registrar.index;
}

}} // end namespace runtime::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR_IMPL__*/
