
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__

#include "common.h"
#include "registry.h"

namespace runtime { namespace auto_registry {

using auto_active_t = simple_function_t;
using auto_active_functor_t = active_function_t;

using auto_active_container_t = std::vector<auto_active_t>;
using auto_active_functor_container_t = std::vector<auto_active_functor_t>;

using auto_handler_t = int32_t;

using handler_manager_t = runtime::HandlerManager;

}} // end namespace runtime::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__*/
