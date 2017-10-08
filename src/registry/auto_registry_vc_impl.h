#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_VC_IMPL__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_VC_IMPL__

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry.h"
#include "context/context_vrt_funcs.h"

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
    ActiveVrtFnPtrType
  >::idx;

  return id;
}

inline AutoActiveVCType getAutoHandlerVC(HandlerType const& handler) {
  return getAutoRegistryGen<AutoActiveVCContainerType>().at(handler).getFun();
}

}} // end namespace vt::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_VC_IMPL__*/
