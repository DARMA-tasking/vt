

#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY__

#include "common.h"
#include "envelope.h"
#include "message.h"
#include "registry.h"

#include <vector>
#include <memory>

namespace runtime { namespace auto_registry {

using auto_active_t = active_function_t;
using auto_active_container_t = std::vector<auto_active_t>;
using auto_handler_t = int32_t;

template <typename = void>
auto_active_container_t&
get_auto_registry()  {
  static auto_active_container_t reg;
  return reg;
}

template <typename ActiveFnT>
struct Registrar {
  auto_handler_t index;

  Registrar() {
    auto_active_container_t& reg = get_auto_registry<>();
    index = reg.size();
    //printf("obj=%p, index=%d\n",this,index);
    auto fn = ActiveFnT::get_function();
    reg.emplace_back(reinterpret_cast<action_basic_function_t*>(fn));
  }
};

inline active_function_t
get_auto_handler(ShortMessage* const msg) {
  handler_t handler = envelope_get_handler(msg->env);
  auto id = the_registry->get_handler_identifier(handler);
  //printf("get_auto_handler:id=%d\n",id);
  return get_auto_registry().at(id);
}

template <typename ActiveFnT>
struct RegistrarWrapper {
  Registrar<ActiveFnT> registrar;
};

template <typename ActiveFnT>
auto_handler_t
register_active_fn() {
  auto const& idx = RegistrarWrapper<ActiveFnT>().registrar.index;
  //printf("idx=%d\n",idx);
  return idx;
}

template <typename F, F* f>
struct FunctorAdapter {
  using function_ptr_type = F;

  static constexpr F* get_function() { return f; }

  template <typename... A>
  auto operator()(A&&... a) -> decltype(f(std::forward<A>(a)...)) {
    return f(std::forward<A>(a)...);
   }
};

template <typename Callable>
struct Runnable {
  using function_ptr_type = typename Callable::function_ptr_type;

  static auto_handler_t const idx;

  static constexpr function_ptr_type*
  get_function() {
    return Callable::get_function();
  }

  Runnable() {
    //printf("Runnable: idx=%d\n",idx);
  }
};

template <typename ActiveFnT>
auto_handler_t const Runnable<ActiveFnT>::idx = register_active_fn<Runnable<ActiveFnT>>();

}} // end namespace runtime::auto_registry

#define register_active_function(F)                                     \
  do {                                                                  \
    runtime::auto_registry::Runnable<decltype(                          \
      runtime::auto_registry::FunctorAdapter<decltype(F),&F>()          \
    )>::idx;                                                            \
  } while (0);

#define get_handler_active_function_expand(TYPE_F, ADD_F)               \
  runtime::auto_registry::Runnable<                                     \
    decltype(                                                           \
      runtime::auto_registry::FunctorAdapter<TYPE_F, ADD_F>()           \
    )>::idx;                                                            \

#define get_handler_active_function(F)                                  \
  get_handler_active_function_expand(decltype(F), std::addressof(F))


#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY__*/
