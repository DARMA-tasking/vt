
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR__

#include "auto_registry_common.h"
#include "common.h"
#include "registry.h"

#include <vector>
#include <memory>

namespace runtime { namespace auto_registry {

template <typename = void>
auto_active_functor_container_t& get_auto_registry_functor();

template <typename FunctorT>
struct RegistrarFunctor {
  auto_handler_t index;

  RegistrarFunctor();
};

auto_active_functor_t get_auto_handler_functor(handler_t const& handler);

template <typename T, typename MessageT>
handler_t make_auto_handler_functor();

template <typename RunnableFunctorT>
struct RegistrarWrapperFunctor {
  RegistrarFunctor<RunnableFunctorT> registrar;
};

template <typename RunnableFunctorT>
auto_handler_t register_active_functor();

template <typename FunctorT, typename MessageT>
struct RunnableFunctor {
  using functor_t = FunctorT;
  using message_t = MessageT;

  static auto_handler_t const idx;

  RunnableFunctor() = default;
};

template <typename FunctorT, typename MessageT>
auto_handler_t const RunnableFunctor<FunctorT, MessageT>::idx =
  register_active_functor<RunnableFunctor<FunctorT, MessageT>>();
}} // end namespace runtime::auto_registry

// convenience macro for registration
#define get_handler_active_functor(FunctorT, MessageT)              \
  runtime::auto_registry::RunnableFunctor<FunctorT, MessageT>::idx;

#include "auto_registry_functor_impl.h"

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR__*/
