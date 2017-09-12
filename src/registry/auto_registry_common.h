
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__

#include "trace_event.h"

#include "common.h"
#include "registry.h"
#include "trace.h"

namespace runtime { namespace auto_registry {

using auto_active_t = simple_function_t;
using auto_active_functor_t = simple_function_t;

template <typename FnT>
struct AutoRegInfo {
  FnT active_fun_t;

  #if backend_check_enabled(trace_enabled)

  trace::TraceEntryIDType event_id;

  AutoRegInfo(
    FnT const& in_active_fun_t,
    trace::TraceEntryIDType const& in_event_id
  ) : active_fun_t(in_active_fun_t), event_id(in_event_id)
  { }

  trace::TraceEntryIDType get_trace_id() const {
    return event_id;
  }

  #else

  AutoRegInfo(
    FnT const& in_active_fun_t
  ) : active_fun_t(in_active_fun_t)
  { }

  #endif

  FnT get_fun() const {
    return active_fun_t;
  }
};

template <typename Fn>
using auto_reg_info_t = AutoRegInfo<Fn>;

using auto_active_container_t = std::vector<auto_reg_info_t<auto_active_t>>;
using auto_active_functor_container_t = std::vector<auto_reg_info_t<auto_active_functor_t>>;

using auto_HandlerType = int32_t;

using handler_manager_t = runtime::HandlerManager;

}} // end namespace runtime::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__*/
