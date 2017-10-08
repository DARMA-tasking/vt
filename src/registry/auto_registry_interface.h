
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_INTERFACE__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_INTERFACE__

#include "auto_registry_common.h"
#include "config.h"
#include "registry.h"

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
trace::TraceEntryIDType getTraceID(HandlerType const& handler);
#endif

}} // end namespace vt::auto_registry

#include "auto_registry.h"
#include "auto_registry_functor.h"

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_INTERFACE__*/
