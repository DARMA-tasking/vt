
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR__

#include "auto_registry_common.h"
#include "common.h"
#include "registry.h"

#include <vector>
#include <memory>

namespace runtime { namespace auto_registry {

using auto_handler_t = int32_t;
using handler_manager_t = runtime::HandlerManager;
using auto_active_functor_t = active_function_t;
using auto_active_functor_container_t = std::vector<auto_active_functor_t>;

template <typename = void>
auto_active_functor_container_t& get_auto_registry_functor();

template <typename FunctorT>
struct RegistrarFunctor {
  auto_handler_t index;

  RegistrarFunctor();
};

auto_active_functor_t get_auto_handler_functor(handler_t const& handler);

template <typename T>
handler_t make_auto_handler_functor();

template <typename RunnableFunctorT>
struct RegistrarWrapperFunctor {
  RegistrarFunctor<RunnableFunctorT> registrar;
};

template <typename FunctorT>
auto_handler_t register_active_functor();

template <typename FunctorT>
struct RunnableFunctor {
  using functor_t = FunctorT;

  static auto_handler_t const idx;

  RunnableFunctor() = default;
};

template <typename FunctorT>
auto_handler_t const RunnableFunctor<FunctorT>::idx =
  register_active_functor<RunnableFunctor<FunctorT>>();

}} // end namespace runtime::auto_registry

#define get_handler_active_function_functor(TYPE_F)                     \
  runtime::auto_registry::RunnableFunctor<TYPE_F>::idx;

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR__*/
