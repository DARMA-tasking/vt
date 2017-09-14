#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_IMPL__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_IMPL__

#include "configs/types/types_common.h"
#include "auto_registry_common.h"
#include "auto_registry.h"

#include <vector>
#include <memory>

namespace vt { namespace auto_registry {

template <typename>
inline AutoActiveContainerType& getAutoRegistry()  {
  static AutoActiveContainerType reg;
  return reg;
}

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
inline HandlerType makeAutoHandler(MessageT* const __attribute__((unused)) msg) {
  HandlerType const id = GET_HANDLER_ACTIVE_FUNCTION_EXPAND(
    ActiveAnyFunctionType<MessageT>, f
  );
  return HandlerManagerType::makeHandler(true, false, id);
}

template <typename T, T value>
inline HandlerType makeAutoHandler() {
  HandlerType const id = GET_HANDLER_ACTIVE_FUNCTION_EXPAND(T, value);
  return HandlerManagerType::makeHandler(true, false, id);
}

template <typename ActiveFnT>
Registrar<ActiveFnT>::Registrar() {
  AutoActiveContainerType& reg = getAutoRegistry<>();
  index = reg.size();
  auto fn = ActiveFnT::getFunction();

  #if backend_check_enabled(trace_enabled)
  auto const& name = demangle::DemanglerUtils::getDemangledType<ActiveFnT>();
  auto const& parsed_names =
    demangle::ActiveFunctionDemangler::parseActiveFunctionName(name);
  auto const& namespace_name = std::get<0>(parsed_names);
  auto const& function_name = std::get<1>(parsed_names);
  auto const& trace_ep = trace::TraceRegistry::registerEventHashed(
    namespace_name, function_name
  );

  reg.emplace_back(AutoRegInfoType<AutoActiveType>{
    reinterpret_cast<ActiveBasicFunctionType*>(fn), trace_ep
  });
  #else
  reg.emplace_back(AutoRegInfoType<AutoActiveType>{
    reinterpret_cast<ActiveBasicFunctionType*>(fn)
  });
  #endif
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

  return getAutoRegistry().at(han_id).getFun();
}

template <typename ActiveFnT>
AutoHandlerType registerActiveFn() {
  return RegistrarWrapper<ActiveFnT>().registrar.index;
}

template <typename Callable>
/*static*/ constexpr typename Runnable<Callable>::FunctionPtrType*
Runnable<Callable>::getFunction() {
  return Callable::getFunction();
}

}} // end namespace vt::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_IMPL__*/
