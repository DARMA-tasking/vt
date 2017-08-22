#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_IMPL__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_IMPL__

#include "common.h"
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
inline handler_t
make_auto_handler(MessageT* const msg) {
  handler_t const id = get_handler_active_function_expand(
    action_any_function_t<MessageT>, f
  );
  return handler_manager_t::make_handler(true, id);
}

template <typename ActiveFnT>
Registrar<ActiveFnT>::Registrar() {
  auto_active_container_t& reg = get_auto_registry<>();
  index = reg.size();
  //printf("obj=%p, index=%d\n",this,index);
  auto fn = ActiveFnT::get_function();
  reg.emplace_back(reinterpret_cast<action_basic_function_t*>(fn));
}

inline active_function_t
get_auto_handler(ShortMessage* const msg) {
  handler_t handler = envelope_get_handler(msg->env);
  auto const& han_id = handler_manager_t::get_handler_identifier(handler);
  return get_auto_registry().at(han_id);
}

template <typename ActiveFnT>
auto_handler_t
register_active_fn() {
  auto const& idx = RegistrarWrapper<ActiveFnT>().registrar.index;
  //printf("idx=%d\n",idx);
  return idx;
}

template <typename Callable>
/*static*/ constexpr typename Runnable<Callable>::function_ptr_type*
Runnable<Callable>::get_function() {
  return Callable::get_function();
}

}} // end namespace runtime::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_IMPL__*/
