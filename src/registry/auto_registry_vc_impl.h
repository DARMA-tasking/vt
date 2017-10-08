#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_VC_IMPL__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_VC_IMPL__

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry.h"
#include "context/context_vrt_funcs.h"

#include <vector>

namespace vt { namespace auto_registry {

template <typename VrtCtxT, typename MsgT, vrt::ActiveVCFunctionType<MsgT, VrtCtxT>* f>
inline HandlerType makeAutoHandlerVC(MsgT* const __attribute__((unused)) msg) {
  HandlerType const id = RunnableGen<decltype(vt::auto_registry::FunctorAdapter<
      vrt::ActiveVCFunctionType<MsgT, VrtCtxT>, f
    >()),
    AutoActiveVCContainerType,
    AutoRegInfoType<AutoActiveVCType>,
    vrt::SimpleVCFunctionType
  >::idx;

  return id;
}

inline AutoActiveVCType getAutoHandlerVC(HandlerType const& handler) {
  return getAutoRegistryGen<AutoActiveVCContainerType>().at(handler).getFun();
}

}} // end namespace vt::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_VC_IMPL__*/
