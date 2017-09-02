
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY__

#include "common.h"
#include "envelope.h"
#include "message.h"
#include "registry.h"

#include <vector>
#include <memory>

namespace runtime { namespace auto_registry {

using auto_active_t = simple_function_t;
using auto_active_container_t = std::vector<auto_active_t>;
using auto_handler_t = int32_t;
using handler_manager_t = runtime::HandlerManager;

using auto_active_functor_t = active_function_t;
using auto_active_functor_container_t = std::vector<auto_active_functor_t>;

template <typename = void>
auto_active_container_t&
get_auto_registry();

template <typename = void>
auto_active_functor_container_t&
get_auto_registry_functor();

template <typename ActiveFnT>
struct Registrar {
  auto_handler_t index;

  Registrar();
};

template <typename FunctorT>
struct RegistrarFunctor {
  auto_handler_t index;

  RegistrarFunctor();
};

auto_active_t
get_auto_handler(handler_t const& handler);

auto_active_functor_t
get_auto_handler_functor(handler_t const& handler);

template <typename MessageT, action_any_function_t<MessageT>* f>
handler_t
make_auto_handler(MessageT* const msg);

template <typename T, T value>
handler_t
make_auto_handler();

template <typename T>
handler_t
make_auto_handler_functor();

template <typename ActiveFnT>
struct RegistrarWrapper {
  Registrar<ActiveFnT> registrar;
};

template <typename RunnableFunctorT>
struct RegistrarWrapperFunctor {
  RegistrarFunctor<RunnableFunctorT> registrar;
};

template <typename ActiveFnT>
auto_handler_t
register_active_fn();

template <typename FunctorT>
auto_handler_t
register_active_functor();

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
  get_function();

  Runnable() = default;
};

template <typename FunctorT>
struct RunnableFunctor {
  using functor_t = FunctorT;

  static auto_handler_t const idx;

  RunnableFunctor() = default;
};

template <typename ActiveFnT>
auto_handler_t const Runnable<ActiveFnT>::idx =
  register_active_fn<Runnable<ActiveFnT>>();

template <typename FunctorT>
auto_handler_t const RunnableFunctor<FunctorT>::idx =
  register_active_functor<RunnableFunctor<FunctorT>>();

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

#define get_handler_active_function_functor(TYPE_F)                     \
  runtime::auto_registry::RunnableFunctor<TYPE_F>::idx;

#define get_handler_active_function(F)                                  \
  get_handler_active_function_expand(decltype(F), std::addressof(F))

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY__*/
