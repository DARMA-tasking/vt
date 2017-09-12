
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__

#include "trace_event.h"

#include "common.h"
#include "registry.h"
#include "trace.h"

namespace runtime { namespace auto_registry {

using AutoActiveType = SimpleFunctionType;
using AutoActiveFunctorType = SimpleFunctionType;

template <typename FnT>
struct AutoRegInfo {
  FnT active_fun_t;

  #if backend_check_enabled(trace_enabled)

  trace::TraceEntryIDType event_id;

  AutoRegInfo(
    FnT const& in_active_fun_t, trace::TraceEntryIDType const& in_event_id
  ) : active_fun_t(in_active_fun_t), event_id(in_event_id)
  { }

  trace::TraceEntryIDType get_trace_id() const {
    return event_id;
  }

  #else

  AutoRegInfo(FnT const& in_active_fun_t)
    : active_fun_t(in_active_fun_t) { }

  #endif

  FnT get_fun() const {
    return active_fun_t;
  }
};

template <typename Fn>
using AutoRegInfoType = AutoRegInfo<Fn>;

using AutoActiveContainerType = std::vector<AutoRegInfoType<AutoActiveType>>;
using AutoActiveFunctorContainerType = std::vector<AutoRegInfoType<AutoActiveFunctorType>>;
using AutoHandlerType = int32_t;
using HandlerManagerType = runtime::HandlerManager;

}} // end namespace runtime::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__*/
