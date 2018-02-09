#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_FUNCTOR_IMPL_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_FUNCTOR_IMPL_H

#include "config.h"
#include "registry/auto_registry_common.h"
#include "registry/auto_registry_functor.h"

#include <vector>
#include <memory>
#include <cassert>

namespace vt { namespace auto_registry {

template <typename FunctorT, bool msg, typename... Args>
inline HandlerType makeAutoHandlerFunctor() {
  using ContainerType = AutoActiveFunctorContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveFunctorType>;
  using FuncType = ActiveFnPtrType;
  using RunType = RunnableFunctor<
    FunctorT, ContainerType, RegInfoType, FuncType, msg, Args...
  >;
  return HandlerManagerType::makeHandler(true, true, RunType::idx);
}

template <typename FunctorT, typename... Args>
static inline auto functorHandlerWrapperRval(Args&&... args) {
  typename FunctorT::FunctorType instance;
  return instance.operator()(std::forward<Args>(args)...);
}

template <typename FunctorT, typename... Args>
static inline auto functorHandlerWrapperReg(Args... args) {
  typename FunctorT::FunctorType instance;
  return instance.operator()(args...);
}

template <
  typename FunctorT, typename RegT, typename InfoT, typename FnT,
  typename... Args
>
static inline void pullApart(
  RegT& reg, bool const& msg,
  pack<Args...> __attribute__((unused)) packed_args
) {
  #if backend_check_enabled(trace_enabled)
  auto const& name = demangle::DemanglerUtils::getDemangledType<
    typename FunctorT::FunctorType
  >();
  auto const& args = demangle::DemanglerUtils::getDemangledType<pack<Args...>>();
  auto const& parsed_names =
    demangle::ActiveFunctorDemangler::parseActiveFunctorName(name, args);
  auto const& namespace_name = std::get<0>(parsed_names);
  auto const& function_name = std::get<1>(parsed_names);
  auto const& trace_ep = trace::TraceRegistry::registerEventHashed(
    namespace_name, function_name
  );
  #endif

  if (msg) {
    auto fn_ptr = functorHandlerWrapperReg<FunctorT, Args...>;
    reg.emplace_back(InfoT{
      reinterpret_cast<FnT>(fn_ptr)
        #if backend_check_enabled(trace_enabled)
        , trace_ep
        #endif
    });
  } else {
    auto fn_ptr = functorHandlerWrapperRval<FunctorT, Args...>;
    reg.emplace_back(InfoT{
      reinterpret_cast<FnT>(fn_ptr)
        #if backend_check_enabled(trace_enabled)
        , trace_ep
        #endif
    });
  }
}

template <typename FunctorT, typename RegT, typename InfoT, typename FnT>
RegistrarFunctor<FunctorT, RegT, InfoT, FnT>::RegistrarFunctor() {
  auto& reg = getAutoRegistryGen<RegT>();
  index = reg.size();

  pullApart<FunctorT, RegT, InfoT, FnT>(
    reg, FunctorT::IsMsgType, typename FunctorT::PackedArgsType()
  );
}

inline AutoActiveFunctorType getAutoHandlerFunctor(HandlerType const& han) {
  auto const& id = HandlerManagerType::getHandlerIdentifier(han);
  bool const& is_auto = HandlerManagerType::isHandlerAuto(han);
  bool const& is_functor = HandlerManagerType::isHandlerFunctor(han);

  debug_print(
    handler, node,
    "getAutoHandlerFunctor: handler=%d, id=%d, is_auto=%s, is_functor=%s\n",
    han, id, print_bool(is_auto), print_bool(is_functor)
  );

  assert(
    (is_functor && is_auto) && "Handler should be auto and functor type!"
  );

  using ContainerType = AutoActiveFunctorContainerType;
  return getAutoRegistryGen<ContainerType>().at(id).getFun();
}

template <typename FunctorT, typename RegT, typename InfoT, typename FnT>
AutoHandlerType registerActiveFunctor() {
  return RegistrarWrapperFunctor<FunctorT, RegT, InfoT, FnT>().registrar.index;
}

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_FUNCTOR_IMPL_H*/
