#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR_IMPL__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR_IMPL__

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry_functor.h"

#include <vector>
#include <memory>

namespace vt { namespace auto_registry {

template <typename FunctorT, bool is_msg, typename... Args>
inline HandlerType makeAutoHandlerFunctor() {
  HandlerType const id = GET_HANDLER_ACTIVE_FUNCTOR(FunctorT, is_msg, Args);
  return HandlerManagerType::makeHandler(true, true, id);
}

template <typename RunnableFunctorT, typename... Args>
static inline void functorHandlerWrapperRval(Args&&... args) {
  typename RunnableFunctorT::FunctorType instance;
  return instance.operator ()(std::forward<Args>(args)...);
}

template <typename RunnableFunctorT, typename... Args>
static inline void functorHandlerWrapperReg(Args... args) {
  typename RunnableFunctorT::FunctorType instance;
  return instance.operator ()(args...);
}

template <typename RunnableFunctorT, typename... Args>
static inline void pullApart(
  AutoActiveFunctorContainerType& reg, bool const& is_msg,
  pack<Args...> __attribute__((unused)) packed_args
) {
  #if backend_check_enabled(trace_enabled)
  auto const& name = demangle::DemanglerUtils::getDemangledType<
    typename RunnableFunctorT::FunctorType
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

  if (is_msg) {
    auto fn_ptr = functorHandlerWrapperReg<RunnableFunctorT, Args...>;
    reg.emplace_back(AutoRegInfoType<AutoActiveFunctorType>{
      reinterpret_cast<SimpleFunctionType>(fn_ptr)
        #if backend_check_enabled(trace_enabled)
        , trace_ep
        #endif
    });
  } else {
    auto fn_ptr = functorHandlerWrapperRval<RunnableFunctorT, Args...>;
    reg.emplace_back(AutoRegInfoType<AutoActiveFunctorType>{
      reinterpret_cast<SimpleFunctionType>(fn_ptr)
        #if backend_check_enabled(trace_enabled)
        , trace_ep
        #endif
    });
  }
}

template <typename RunnableFunctorT>
RegistrarFunctor<RunnableFunctorT>::RegistrarFunctor() {
  AutoActiveFunctorContainerType& reg =
    getAutoRegistryGen<AutoActiveFunctorContainerType>();
  index = reg.size();

  pullApart<RunnableFunctorT>(
    reg, RunnableFunctorT::IsMsgType, typename RunnableFunctorT::PackedArgsType()
  );
}

inline AutoActiveFunctorType getAutoHandlerFunctor(HandlerType const& handler) {
  auto const& han_id = HandlerManagerType::getHandlerIdentifier(handler);

  bool const& is_auto = HandlerManagerType::isHandlerAuto(handler);
  bool const& is_functor = HandlerManagerType::isHandlerFunctor(handler);

  debug_print(
    handler, node,
    "get_auto_handler: handler=%d, id=%d, is_auto=%s, is_functor=%s\n",
    handler, han_id, print_bool(is_auto), print_bool(is_functor)
  );

  assert(
    is_functor and is_auto and "Handler should be a auto functor"
  );

  return getAutoRegistryGen<AutoActiveFunctorContainerType>().at(han_id).getFun();
}

template <typename RunnableFunctorT>
AutoHandlerType registerActiveFunctor() {
  return RegistrarWrapperFunctor<RunnableFunctorT>().registrar.index;
}

}} // end namespace vt::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_FUNCTOR_IMPL__*/
