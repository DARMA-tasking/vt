#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_VC_IMPL_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_VC_IMPL_H

#include "config.h"
#include "registry/auto/auto_registry_common.h"
#include "registry/auto/auto_registry.h"
#include "vrt/context/context_vrt_funcs.h"

#include <vector>

namespace vt { namespace auto_registry {

using namespace vrt;

template <typename VrtT, typename MsgT, ActiveVrtTypedFnType<MsgT, VrtT>* f>
inline HandlerType makeAutoHandlerVC(MsgT* const __attribute__((unused)) msg) {
  using FunctorT = FunctorAdapter<ActiveVrtTypedFnType<MsgT, VrtT>, f>;
  using ContainerType = AutoActiveVCContainerType;
  using RegInfoType = AutoRegInfoType<AutoActiveVCType>;
  using FuncType = ActiveVirtualFnPtrType;
  return RunnableGen<FunctorT, ContainerType, RegInfoType, FuncType>::idx;
}

inline AutoActiveVCType getAutoHandlerVC(HandlerType const& handler) {
  using ContainerType = AutoActiveVCContainerType;
  return getAutoRegistryGen<ContainerType>().at(handler).getFun();
}

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_VC_IMPL_H*/
