
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_INTERFACE__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_INTERFACE__

#include "auto_registry_common.h"
#include "common.h"
#include "registry.h"

namespace runtime { namespace auto_registry {

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
HandlerType make_auto_handler(MessageT* const msg);

template <typename T, T value>
HandlerType make_auto_handler();

template <typename T, bool is_msg, typename... Args>
HandlerType make_auto_handler_functor();

AutoActiveType get_auto_handler(HandlerType const& handler);

AutoActiveFunctorType get_auto_handler_functor(HandlerType const& handler);

#if backend_check_enabled(trace_enabled)
trace::TraceEntryIDType get_trace_id(HandlerType const& handler);
#endif

}} // end namespace runtime::auto_registry

#include "auto_registry.h"
#include "auto_registry_functor.h"

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_INTERFACE__*/
