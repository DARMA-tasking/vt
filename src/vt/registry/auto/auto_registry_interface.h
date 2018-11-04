
#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_INTERFACE_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_INTERFACE_H

#include "vt/registry/auto/auto_registry_common.h"
#include "vt/config.h"
#include "vt/registry/registry.h"

namespace vt { namespace auto_registry {

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
HandlerType makeAutoHandler(MessageT* const msg);

template <typename T, T value>
HandlerType makeAutoHandler();

template <typename T, bool is_msg, typename... Args>
HandlerType makeAutoHandlerFunctor();

AutoActiveType getAutoHandler(HandlerType const& handler);

AutoActiveFunctorType getAutoHandlerFunctor(HandlerType const& handler);

#if backend_check_enabled(trace_enabled)
  trace::TraceEntryIDType theTraceID(
    HandlerType const& handler, RegistryTypeEnum reg_type
  );
#endif

}} // end namespace vt::auto_registry

#include "vt/registry/auto/auto_registry.h"
#include "vt/registry/auto/functor/auto_registry_functor.h"
#include "vt/registry/auto/index/auto_registry_index.h"

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_INTERFACE_H*/
