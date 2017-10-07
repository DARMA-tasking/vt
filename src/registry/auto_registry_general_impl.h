
#if !defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_GENERAL_IMPL__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_GENERAL_IMPL__

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry_general.h"

namespace vt { namespace auto_registry {

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
RegistrarGen<ActFnT, RegT, InfoT, FnT>::RegistrarGen() {
  RegT& reg = getAutoRegistryGen<RegT>();
  index = reg.size();

  auto fn = ActFnT::getFunction();

  #if backend_check_enabled(trace_enabled)
  auto const& name = demangle::DemanglerUtils::getDemangledType<ActFnT>();
  auto const& parsed_names =
    demangle::ActiveFunctionDemangler::parseActiveFunctionName(name);
  auto const& namespace_name = std::get<0>(parsed_names);
  auto const& function_name = std::get<1>(parsed_names);
  auto const& trace_ep = trace::TraceRegistry::registerEventHashed(
    namespace_name, function_name
  );

  reg.emplace_back(InfoT{reinterpret_cast<FnT>(fn), trace_ep});
  #else
  reg.emplace_back(InfoT{reinterpret_cast<FnT>(fn)});
  #endif
}

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
AutoHandlerType registerActiveGen() {
  return RegistrarWrapperGen<ActFnT, RegT, InfoT, FnT>().registrar.index;
}

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
/*static*/ constexpr typename
RunnableGen<ActFnT, RegT, InfoT, FnT>::FunctionPtrType*
RunnableGen<ActFnT, RegT, InfoT, FnT>::getFunction() {
  return ActFnT::getFunction();
}

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
AutoHandlerType const RunnableGen<ActFnT, RegT, InfoT, FnT>::idx =
  registerActiveGen<RunnableGen<ActFnT, RegT, InfoT, FnT>, RegT, InfoT, FnT>();

}} /* end namespace vt::auto_registry */

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_GENERAL_IMPL__*/
