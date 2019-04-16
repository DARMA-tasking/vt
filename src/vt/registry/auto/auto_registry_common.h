/*
//@HEADER
// ************************************************************************
//
//                          auto_registry_common.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_COMMON_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_COMMON_H

#include "vt/trace/trace_event.h"

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/registry/registry.h"
#include "vt/trace/trace.h"
#include "vt/vrt/context/context_vrt_funcs.h"
#include "vt/vrt/collection/active/active_funcs.h"
#include "vt/topos/mapping/mapping_function.h"
#include "vt/objgroup/active_func/active_func.h"

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
using AutoActiveObjGroupType      = objgroup::ActiveObjAnyType;

using HandlerManagerType = vt::HandlerManager;
using AutoHandlerType = int32_t;
using NumArgsType = int16_t;

enum struct RegistryTypeEnum {
  RegGeneral = 1,
  RegMap,
  RegVrt,
  RegSeed,
  RegVrtCollection,
  RegVrtCollectionMember,
  RegRDMAGet,
  RegRDMAPut,
  RegIndex,
  RegObjGroup
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static struct NumArgsTagType { } NumArgsTag { };
#pragma GCC diagnostic pop

template <typename FnT>
struct AutoRegInfo {
  FnT activeFunT;
  NumArgsType args_ = 1;
  AutoHandlerType obj_idx_ = -1;

  #if backend_check_enabled(trace_enabled)
    trace::TraceEntryIDType event_id;
    AutoRegInfo(
      FnT const& in_active_fun_t, AutoHandlerType in_obj_idx,
      trace::TraceEntryIDType const& in_event_id
    ) : activeFunT(in_active_fun_t), obj_idx_(in_obj_idx), event_id(in_event_id)
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
    explicit AutoRegInfo(FnT const& in_active_fun_t, AutoHandlerType in_obj_idx)
      : activeFunT(in_active_fun_t), obj_idx_(in_obj_idx)
    { }
    AutoRegInfo(
      NumArgsTagType,
      FnT const& in_active_fun_t, NumArgsType const& in_args
    ) : activeFunT(in_active_fun_t), args_(in_args)
    { }
  #endif

  AutoHandlerType getObjIdx() const {
    return obj_idx_;
  }

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
using AutoActiveObjGroupContainerType      = RegContType<AutoActiveObjGroupType>;

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_COMMON_H*/
