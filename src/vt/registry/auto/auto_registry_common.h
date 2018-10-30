
#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_COMMON_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_COMMON_H

#include "trace/trace_event.h"

#include "config.h"
#include "activefn/activefn.h"
#include "registry/registry.h"
#include "trace/trace.h"
#include "vrt/context/context_vrt_funcs.h"
#include "vrt/collection/active/active_funcs.h"
#include "topos/mapping/mapping_function.h"

#include <vector>
#include <cstdlib>

namespace vt { namespace auto_registry {

using AutoActiveType              = ActiveFnPtrType;
using AutoActiveFunctorType       = ActiveFnPtrType;
using AutoActiveVCType            = vrt::ActiveVirtualFnPtrType;
using AutoActiveCollectionType    = vrt::collection::ActiveColFnPtrType;
using AutoActiveCollectionMemType = vrt::collection::ActiveColMemberFnPtrType;
using AutoActiveMapType           = mapping::ActiveMapFnPtrType;
using AutoActiveMapFunctorType    = mapping::ActiveMapFnPtrType;
using AutoActiveSeedMapType       = mapping::ActiveSeedMapFnPtrType;
using AutoActiveRDMAGetType       = ActiveRDMAGetFnPtrType;
using AutoActiveRDMAPutType       = ActiveRDMAPutFnPtrType;
using AutoActiveIndexType         = std::size_t;

using HandlerManagerType = vt::HandlerManager;
using AutoHandlerType = int32_t;
using NumArgsType = int16_t;

enum struct RegistryTypeEnum {
  RegGeneral = 1,
  RegMap,
  RegVrt,
  RegSeed,
  RegVrtCollection,
  RegRDMAGet,
  RegRDMAPut,
  RegIndex
};

static struct NumArgsTagType { } NumArgsTag { };

template <typename FnT>
struct AutoRegInfo {
  FnT activeFunT;
  NumArgsType args_ = 1;

  #if backend_check_enabled(trace_enabled)
    trace::TraceEntryIDType event_id;
    AutoRegInfo(
      FnT const& in_active_fun_t, trace::TraceEntryIDType const& in_event_id
    ) : activeFunT(in_active_fun_t), event_id(in_event_id)
    { }
    AutoRegInfo(
      NumArgsTagType,
      FnT const& in_active_fun_t, trace::TraceEntryIDType const& in_event_id,
      NumArgsType const& in_args
    ) : activeFunT(in_active_fun_t), event_id(in_event_id), args_(in_args)
    { }
    trace::TraceEntryIDType theTraceID() const {
      return event_id;
    }
  #else
    explicit AutoRegInfo(FnT const& in_active_fun_t)
      : activeFunT(in_active_fun_t)
    { }
    AutoRegInfo(
      NumArgsTagType,
      FnT const& in_active_fun_t, NumArgsType const& in_args
    ) : activeFunT(in_active_fun_t), args_(in_args)
    { }
  #endif

  NumArgsType getNumArgs() const {
    return args_;
  }

  FnT getFun() const {
    return activeFunT;
  }
};

template <typename Fn>
using AutoRegInfoType = AutoRegInfo<Fn>;

template <typename RegInfoT>
using RegContType = std::vector<AutoRegInfoType<RegInfoT>>;

using AutoActiveContainerType              = RegContType<AutoActiveType>;
using AutoActiveVCContainerType            = RegContType<AutoActiveVCType>;
using AutoActiveCollectionContainerType    = RegContType<AutoActiveCollectionType>;
using AutoActiveCollectionMemContainerType = RegContType<AutoActiveCollectionMemType>;
using AutoActiveMapContainerType           = RegContType<AutoActiveMapType>;
using AutoActiveMapFunctorContainerType    = RegContType<AutoActiveMapFunctorType>;
using AutoActiveSeedMapContainerType       = RegContType<AutoActiveSeedMapType>;
using AutoActiveFunctorContainerType       = RegContType<AutoActiveFunctorType>;
using AutoActiveRDMAGetContainerType       = RegContType<AutoActiveRDMAGetType>;
using AutoActiveRDMAPutContainerType       = RegContType<AutoActiveRDMAPutType>;
using AutoActiveIndexContainerType         = RegContType<AutoActiveIndexType>;

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_COMMON_H*/
