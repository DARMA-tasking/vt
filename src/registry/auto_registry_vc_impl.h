#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_VC_IMPL__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_VC_IMPL__

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry.h"

#include <vector>

namespace vt { namespace auto_registry {

template <typename>
inline AutoActiveVCContainerType& getAutoRegistryVC()  {
  static AutoActiveVCContainerType reg;
  return reg;
}

template <
  typename VirtualContextT,
  typename MessageT,
  ActiveVCFunctionType<MessageT, VirtualContextT>* f
>
inline HandlerType makeAutoHandlerVC(MessageT* const __attribute__((unused)) msg) {
  HandlerType const id = RunnableVC<decltype(vt::auto_registry::FunctorAdapterVC<
    ActiveVCFunctionType<MessageT, VirtualContextT>, f
  >())>::idx;

  return id;
}

template <typename ActiveFnT>
RegistrarVC<ActiveFnT>::RegistrarVC() {
  AutoActiveVCContainerType& reg = getAutoRegistryVC<>();
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

  reg.emplace_back(AutoRegInfoType<AutoActiveVCType>{
    reinterpret_cast<SimpleVCFunctionType>(fn), trace_ep
  });
  #else
  reg.emplace_back(AutoRegInfoType<AutoActiveVCType>{
    reinterpret_cast<SimpleVCFunctionType>(fn)
  });
  #endif
}

inline AutoActiveVCType getAutoHandlerVC(HandlerType const& handler) {
  return getAutoRegistryVC().at(handler).getFun();
}

template <typename ActiveFnT>
AutoHandlerType registerActiveFnVC() {
  return RegistrarWrapperVC<ActiveFnT>().registrar.index;
}

template <typename Callable>
/*static*/ constexpr typename RunnableVC<Callable>::FunctionPtrType*
RunnableVC<Callable>::getFunction() {
  return Callable::getFunction();
}

}} // end namespace vt::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_VC_IMPL__*/
