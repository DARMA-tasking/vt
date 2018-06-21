
#include "config.h"
#include "registry/auto/auto_registry_common.h"
#include "registry/auto/auto_registry_interface.h"
#include "registry/registry.h"

#include <cassert>

namespace vt { namespace auto_registry {

#if backend_check_enabled(trace_enabled)
trace::TraceEntryIDType theTraceID(
  HandlerType const& handler, RegistryTypeEnum reg_type
) {
  switch (reg_type) {
  case RegistryTypeEnum::RegGeneral: {
    bool const& is_functor = HandlerManagerType::isHandlerFunctor(handler);
    auto const& han_id = HandlerManagerType::getHandlerIdentifier(handler);
    if (not is_functor) {
      using ContType = AutoActiveContainerType;
      return getAutoRegistryGen<ContType>().at(han_id).theTraceID();
    } else {
      using ContType = AutoActiveFunctorContainerType;
      return getAutoRegistryGen<ContType>().at(han_id).theTraceID();
    }
    break;
  }
  case RegistryTypeEnum::RegMap: {
    bool const& is_functor = HandlerManagerType::isHandlerFunctor(handler);
    auto const& han_id = HandlerManagerType::getHandlerIdentifier(handler);
    if (not is_functor) {
      using ContType = AutoActiveMapContainerType;
      return getAutoRegistryGen<ContType>().at(han_id).theTraceID();
    } else {
      using ContType = AutoActiveMapFunctorContainerType;
      auto const& han_id = HandlerManagerType::getHandlerIdentifier(handler);
      return getAutoRegistryGen<ContType>().at(han_id).theTraceID();
    }
    break;
  }
  case RegistryTypeEnum::RegVrt: {
    using ContType = AutoActiveVCContainerType;
    return getAutoRegistryGen<ContType>().at(handler).theTraceID();
    break;
  }
  case RegistryTypeEnum::RegVrtCollection: {
    using ContType = AutoActiveCollectionContainerType;
    return getAutoRegistryGen<ContType>().at(handler).theTraceID();
    break;
  }
  case RegistryTypeEnum::RegSeed: {
    using ContType = AutoActiveSeedMapContainerType;
    auto const& han_id = HandlerManagerType::getHandlerIdentifier(handler);
    return getAutoRegistryGen<ContType>().at(han_id).theTraceID();
    break;
  }
  default:
    assert(0 && "Should not be reachable");
    break;
  }
}
#endif

}} // end namespace vt::auto_registry
