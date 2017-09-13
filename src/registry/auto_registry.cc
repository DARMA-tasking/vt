
#include "auto_registry_common.h"
#include "auto_registry_interface.h"
#include "common.h"
#include "registry.h"

namespace vt { namespace auto_registry {

#if backend_check_enabled(trace_enabled)
trace::TraceEntryIDType
get_trace_id(HandlerType const& handler) {
  auto const& han_id = HandlerManagerType::getHandlerIdentifier(handler);

  bool const& is_functor = HandlerManagerType::isHandlerFunctor(handler);

  if (not is_functor) {
    return getAutoRegistry().at(han_id).get_trace_id();
  } else {
    return getAutoRegistryFunctor().at(han_id).get_trace_id();
  }
}
#endif

}} // end namespace vt::auto_registry
