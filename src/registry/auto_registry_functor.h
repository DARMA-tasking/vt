
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR__

#include "auto_registry_common.h"
#include "config.h"
#include "registry.h"

#include <vector>
#include <memory>

namespace vt { namespace auto_registry {

template <typename FunctorT>
struct RegistrarFunctor {
  AutoHandlerType index;

  RegistrarFunctor();
};

AutoActiveFunctorType getAutoHandlerFunctor(HandlerType const& handler);

template <typename FunctorT, bool is_msg, typename... Args>
HandlerType makeAutoHandlerFunctor();

template <typename RunnableFunctorT>
struct RegistrarWrapperFunctor {
  RegistrarFunctor<RunnableFunctorT> registrar;
};

template <typename RunnableFunctorT>
AutoHandlerType registerActiveFunctor();

template <typename... Args>
struct pack { };

template <typename FunctorT, bool is_msg, typename... Args>
struct RunnableFunctor {
  using FunctorType = FunctorT;
  using PackedArgsType = pack<Args...>;

  static constexpr bool const IsMsgType = is_msg;

  static AutoHandlerType const idx;

  RunnableFunctor() = default;
};

template <typename FunctorT, bool is_msg, typename... Args>
AutoHandlerType const RunnableFunctor<FunctorT, is_msg, Args...>::idx =
  registerActiveFunctor<RunnableFunctor<FunctorT, is_msg, Args...>>();

template <typename FunctorT, bool is_msg, typename... Args>
bool const RunnableFunctor<FunctorT, is_msg, Args...>::IsMsgType;

}} // end namespace vt::auto_registry

// convenience macro for registration
#define GET_HANDLER_ACTIVE_FUNCTOR(FunctorT, is_msg, Args)          \
  vt::auto_registry::RunnableFunctor<FunctorT, is_msg, Args...>::idx;

#include "auto_registry_functor_impl.h"

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR__*/
