#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_VC_IMPL_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_VC_IMPL_H

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry.h"
#include "vrt/context/context_vrt_funcs.h"

#include <vector>

namespace vt { namespace auto_registry {

using namespace vrt;

template <typename VrtT, typename MsgT, ActiveVrtTypedFnType<MsgT, VrtT>* f>
inline HandlerType makeAutoHandlerVC(MsgT* const __attribute__((unused)) msg) {
  HandlerType const id = RunnableGen<decltype(vt::auto_registry::FunctorAdapter<
      ActiveVrtTypedFnType<MsgT, VrtT>, f
    >()),
    AutoActiveVCContainerType,
    AutoRegInfoType<AutoActiveVCType>,
    ActiveVirtualFnPtrType
  >::idx;

  return id;
}

inline AutoActiveVCType getAutoHandlerVC(HandlerType const& handler) {
  return getAutoRegistryGen<AutoActiveVCContainerType>().at(handler).getFun();
}

}} // end namespace vt::auto_registry

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_VC_IMPL_H*/
