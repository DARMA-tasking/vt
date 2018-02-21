#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_IMPL_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_IMPL_H

#include "config.h"
#include "registry/auto/auto_registry_common.h"
#include "registry/auto/auto_registry.h"

#include <vector>
#include <memory>

namespace vt { namespace auto_registry {

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
inline HandlerType makeAutoHandler(MessageT* const __attribute__((unused)) msg) {
  using FunctorT = FunctorAdapter<ActiveTypedFnType<MessageT>, f>;
  using ContainerType = AutoActiveContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveType>;
  using FuncType = ActiveFnPtrType;
  using RunType = RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>;
  return HandlerManagerType::makeHandler(true, false, RunType::idx);
}

template <typename T, T value>
inline HandlerType makeAutoHandler() {
  using FunctorT = FunctorAdapter<T, value>;
  using ContainerType = AutoActiveContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveType>;
  using FuncType = ActiveFnPtrType;
  using RunType = RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>;
  return HandlerManagerType::makeHandler(true, false, RunType::idx);
}

inline AutoActiveType getAutoHandler(HandlerType const& handler) {
  auto const& han_id = HandlerManagerType::getHandlerIdentifier(handler);
  bool const& is_auto = HandlerManagerType::isHandlerAuto(handler);
  bool const& is_functor = HandlerManagerType::isHandlerFunctor(handler);

  debug_print(
    handler, node,
    "get_auto_handler: handler=%d, id=%d, is_auto=%s, is_functor=%s\n",
    handler, han_id, print_bool(is_auto), print_bool(is_functor)
  );

  assert(
    not is_functor and is_auto and "Handler should not be a functor, but auto"
  );

  return getAutoRegistryGen<AutoActiveContainerType>().at(han_id).getFun();
}

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_IMPL_H*/
