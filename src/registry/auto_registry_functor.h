
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR__

#include "auto_registry_common.h"
#include "common.h"
#include "registry.h"

#include <vector>
#include <memory>

namespace runtime { namespace auto_registry {

template <typename = void>
AutoActiveFunctorContainerType& get_auto_registry_functor();

template <typename FunctorT>
struct RegistrarFunctor {
  AutoHandlerType index;

  RegistrarFunctor();
};

AutoActiveFunctorType get_auto_handler_functor(HandlerType const& handler);

template <typename T, bool is_msg, typename... Args>
HandlerType make_auto_handler_functor();

template <typename RunnableFunctorT>
struct RegistrarWrapperFunctor {
  RegistrarFunctor<RunnableFunctorT> registrar;
};

template <typename RunnableFunctorT>
AutoHandlerType register_active_functor();

template <typename... Args>
struct pack { };

template <typename FunctorT, bool is_msg, typename... Args>
struct RunnableFunctor {
  using functor_t = FunctorT;
  using packed_args_t = pack<Args...>;

  static constexpr bool const is_msg_t = is_msg;

  static AutoHandlerType const idx;

  RunnableFunctor() = default;
};

template <typename FunctorT, bool is_msg, typename... Args>
AutoHandlerType const RunnableFunctor<FunctorT, is_msg, Args...>::idx =
  register_active_functor<RunnableFunctor<FunctorT, is_msg, Args...>>();

template <typename FunctorT, bool is_msg, typename... Args>
bool const RunnableFunctor<FunctorT, is_msg, Args...>::is_msg_t;

}} // end namespace runtime::auto_registry

// convenience macro for registration
#define get_handler_active_functor(FunctorT, is_msg, Args)          \
  runtime::auto_registry::RunnableFunctor<FunctorT, is_msg, Args...>::idx;

#include "auto_registry_functor_impl.h"

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR__*/
