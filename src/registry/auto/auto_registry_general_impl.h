
#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_IMPL_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_IMPL_H

#include "config.h"
#include "utils/demangle/demangle.h"
#include "registry/auto/auto_registry_common.h"
#include "registry/auto/auto_registry_general.h"

namespace vt { namespace auto_registry {

template <typename ActFnT, typename RegT, typename InfoT, typename FnT>
RegistrarGen<ActFnT, RegT, InfoT, FnT>::RegistrarGen() {
  RegT& reg = getAutoRegistryGen<RegT>();
  index = reg.size();

  auto fn = ActFnT::getFunction();

  #if backend_check_enabled(trace_enabled)
  using Tn = typename ActFnT::ActFnType;
  auto const& type_name = util::demangle::DemanglerUtils::getTypeName<Tn>();
  auto const& parsed_type_name =
    util::demangle::ActiveFunctionDemangler::parseActiveFunctionName(type_name);
  auto const& trace_ep = trace::TraceRegistry::registerEventHashed(
    parsed_type_name.getNamespace(), parsed_type_name.getFuncParams()
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

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_GENERAL_IMPL_H*/
