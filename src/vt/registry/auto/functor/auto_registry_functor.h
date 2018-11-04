
#if !defined INCLUDED_REGISTRY_AUTO_FUNCTOR_AUTO_REGISTRY_FUNCTOR_H
#define INCLUDED_REGISTRY_AUTO_FUNCTOR_AUTO_REGISTRY_FUNCTOR_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/registry.h"

#include <vector>
#include <memory>

namespace vt { namespace auto_registry {

template <typename FunctorT, typename RegT, typename InfoT, typename FnT>
struct RegistrarFunctor {
  AutoHandlerType index;

  RegistrarFunctor();
};

AutoActiveFunctorType getAutoHandlerFunctor(HandlerType const& handler);
NumArgsType getAutoHandlerFunctorArgs(HandlerType const& handler);

template <typename FunctorT, bool is_msg, typename... Args>
HandlerType makeAutoHandlerFunctor();

template <typename FunctorT, typename RegT, typename InfoT, typename FnT>
struct RegistrarWrapperFunctor {
  RegistrarFunctor<FunctorT, RegT, InfoT, FnT> registrar;
};

template <typename FunctorT, typename RegT, typename InfoT, typename FnT>
AutoHandlerType registerActiveFunctor();

template <typename... Args>
struct pack { };

template <
  typename FunctorT, typename RegT, typename InfoT, typename FnT, bool msg,
  typename... Args
>
struct RunnableFunctor {
  using FunctorType = FunctorT;
  using PackedArgsType = pack<Args...>;

  static constexpr bool const IsMsgType = msg;

  static AutoHandlerType const idx;

  RunnableFunctor() = default;
};

template <
  typename FunctorT, typename RegT, typename InfoT, typename FnT, bool msg,
  typename... Args
>
AutoHandlerType const RunnableFunctor<FunctorT,RegT,InfoT,FnT,msg,Args...>::idx =
  registerActiveFunctor<
  RunnableFunctor<FunctorT, RegT, InfoT, FnT, msg, Args...>, RegT, InfoT, FnT
  >();

template <
  typename FunctorT, typename RegT, typename InfoT, typename FnT, bool msg,
  typename... Args
>
bool const RunnableFunctor<FunctorT, RegT, InfoT, FnT, msg, Args...>::IsMsgType;

}} // end namespace vt::auto_registry

#include "vt/registry/auto/functor/auto_registry_functor_impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_FUNCTOR_AUTO_REGISTRY_FUNCTOR_H*/
