
#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_COMMON_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_COMMON_H

#include "trace/trace_event.h"

#include "config.h"
#include "activefn/activefn.h"
#include "registry/registry.h"
#include "trace/trace.h"
#include "vrt/context/context_vrt_funcs.h"
#include "vrt/collection/collection_active_funcs.h"
#include "topos/mapping/mapping_function.h"

#include <vector>

namespace vt { namespace auto_registry {

using AutoActiveType           = ActiveFnPtrType;
using AutoActiveFunctorType    = ActiveFnPtrType;
using AutoActiveVCType         = vrt::ActiveVirtualFnPtrType;
using AutoActiveCollectionType = vrt::collection::ActiveColFnPtrType;
using AutoActiveMapType        = mapping::ActiveMapFnPtrType;
using AutoActiveMapFunctorType = mapping::ActiveMapFnPtrType;
using AutoActiveSeedMapType    = mapping::ActiveSeedMapFnPtrType;

using HandlerManagerType = vt::HandlerManager;
using AutoHandlerType = int32_t;

template <typename FnT>
struct AutoRegInfo {
  FnT activeFunT;

  #if backend_check_enabled(trace_enabled)
    trace::TraceEntryIDType event_id;
    AutoRegInfo(
      FnT const& in_active_fun_t, trace::TraceEntryIDType const& in_event_id
    ) : activeFunT(in_active_fun_t), event_id(in_event_id)
    { }
    trace::TraceEntryIDType theTraceID() const {
      return event_id;
    }
  #else
    explicit AutoRegInfo(FnT const& in_active_fun_t)
      : activeFunT(in_active_fun_t)
    { }
  #endif

  FnT getFun() const {
    return activeFunT;
  }
};

template <typename Fn>
using AutoRegInfoType = AutoRegInfo<Fn>;

template <typename RegInfoT>
using RegContType = std::vector<AutoRegInfoType<RegInfoT>>;

using AutoActiveContainerType           = RegContType<AutoActiveType>;
using AutoActiveVCContainerType         = RegContType<AutoActiveVCType>;
using AutoActiveCollectionContainerType = RegContType<AutoActiveCollectionType>;
using AutoActiveMapContainerType        = RegContType<AutoActiveMapType>;
using AutoActiveMapFunctorContainerType = RegContType<AutoActiveMapFunctorType>;
using AutoActiveSeedMapContainerType    = RegContType<AutoActiveSeedMapType>;
using AutoActiveFunctorContainerType    = RegContType<AutoActiveFunctorType>;

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_COMMON_H*/
