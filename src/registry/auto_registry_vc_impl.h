#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_VC_IMPL__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_VC_IMPL__

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry.h"

#include <vector>

namespace vt { namespace auto_registry {

template <
  typename VirtualContextT,
  typename MessageT,
  ActiveVCFunctionType<MessageT, VirtualContextT>* f
>
inline HandlerType makeAutoHandlerVC(MessageT* const __attribute__((unused)) msg) {
  HandlerType const id = RunnableGen<
    decltype(vt::auto_registry::FunctorAdapter<
      ActiveVCFunctionType<MessageT, VirtualContextT>, f
    >()),
    AutoActiveVCContainerType, AutoRegInfoType<AutoActiveVCType>,
    SimpleVCFunctionType
  >::idx;

  return id;
}

inline AutoActiveVCType getAutoHandlerVC(HandlerType const& handler) {
  return getAutoRegistryGen<AutoActiveVCContainerType>().at(handler).getFun();
}

}} // end namespace vt::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_VC_IMPL__*/
