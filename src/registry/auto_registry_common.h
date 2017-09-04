
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__

#include "trace_event.h"

#include "common.h"
#include "registry.h"
#include "trace.h"

namespace runtime { namespace auto_registry {

using auto_active_t = simple_function_t;
using auto_active_functor_t = simple_function_t;

struct AutoRegInfo {
  auto_active_t active_fun_t;

  #if backend_check_enabled(trace_enabled)

  trace::trace_event_id_t event_id;

  AutoRegInfo(
    auto_active_t const& in_active_fun_t,
    trace::trace_event_id_t const& in_event_id
  ) : active_fun_t(in_active_fun_t), event_id(in_event_id)
  { }

  trace::trace_event_id_t get_trace_id() const {
    return event_id;
  }

  #else

  AutoRegInfo(
    auto_active_t const& in_active_fun_t
  ) : active_fun_t(in_active_fun_t)
  { }

  #endif

  auto_active_t get_fun() const {
    return active_fun_t;
  }
};

using auto_reg_info_t = AutoRegInfo;

using auto_active_container_t = std::vector<auto_reg_info_t>;
using auto_active_functor_container_t = std::vector<auto_active_functor_t>;

using auto_handler_t = int32_t;

using handler_manager_t = runtime::HandlerManager;

}} // end namespace runtime::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__*/
