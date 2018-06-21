#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_H

#include "config.h"
#include "registry/auto/auto_registry_common.h"

namespace vt { namespace auto_registry {

template <typename F, F* f>
struct FunctorAdapter {
  using FunctionPtrType = F;

  static constexpr F* getFunction() { return f; }

  template <typename... A>
  auto operator()(A&&... a) -> decltype(f(std::forward<A>(a)...)) {
    return f(std::forward<A>(a)...);
   }
};

template <typename RegistryT, typename = void>
RegistryT& getAutoRegistryGen();

template <typename RegistryT, typename>
inline RegistryT& getAutoRegistryGen() {
#pragma sst keep
  static RegistryT reg;
  return reg;
}

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
struct RegistrarGen {
  AutoHandlerType index;

  RegistrarGen();
};

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
struct RegistrarWrapperGen {
  RegistrarGen<ActFnT, RegT, InfoT, FnT> registrar;
};

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
AutoHandlerType registerActiveGen();

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
struct RunnableGen {
  using ActFnType = ActFnT;
  using FunctionPtrType = typename ActFnT::FunctionPtrType;

  static AutoHandlerType const idx;
  static constexpr FunctionPtrType* getFunction();

  RunnableGen() = default;
};

}} // end namespace vt::auto_registry

#include "registry/auto/auto_registry_general_impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_H*/
