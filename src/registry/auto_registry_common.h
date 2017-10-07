
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__

#include "trace_event.h"

#include "config.h"
#include "registry_function.h"
#include "registry.h"
#include "trace.h"
#include "context_vrt_fwd.h"
#include "mapping_function.h"

namespace vt { namespace auto_registry {

using AutoActiveType = SimpleFunctionType;
using AutoActiveVCType = SimpleVCFunctionType;
using AutoActiveMapType = mapping::SimpleMapFunctionType;
using AutoActiveFunctorType = SimpleFunctionType;

template <typename FnT>
struct AutoRegInfo {
  FnT activeFunT;

  #if backend_check_enabled(trace_enabled)

  trace::TraceEntryIDType event_id;

  AutoRegInfo(
    FnT const& in_active_fun_t, trace::TraceEntryIDType const& in_event_id
  ) : activeFunT(in_active_fun_t), event_id(in_event_id)
  { }

  trace::TraceEntryIDType getTraceID() const {
    return event_id;
  }

  #else

  AutoRegInfo(FnT const& in_active_fun_t)
    : activeFunT(in_active_fun_t) { }

  #endif

  FnT getFun() const {
    return activeFunT;
  }
};

template <typename Fn>
using AutoRegInfoType = AutoRegInfo<Fn>;

using AutoActiveContainerType = std::vector<AutoRegInfoType<AutoActiveType>>;
using AutoActiveVCContainerType = std::vector<AutoRegInfoType<AutoActiveVCType>>;
using AutoActiveMapContainerType = std::vector<AutoRegInfoType<AutoActiveMapType>>;
using AutoActiveFunctorContainerType = std::vector<AutoRegInfoType<AutoActiveFunctorType>>;
using AutoHandlerType = int32_t;
using HandlerManagerType = vt::HandlerManager;

}} // end namespace vt::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_COMMON__*/
