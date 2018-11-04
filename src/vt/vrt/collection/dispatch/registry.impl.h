
#if !defined INCLUDED_VRT_COLLECTION_DISPATCH_REGISTRY_IMPL_H
#define INCLUDED_VRT_COLLECTION_DISPATCH_REGISTRY_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/dispatch/dispatch.h"
#include "vt/vrt/collection/dispatch/registry.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/registry/auto/auto_registry_common.h"

namespace vt { namespace vrt { namespace collection {

inline RegistryTLType& getTLRegistry() {
  #pragma sst keep
  static RegistryTLType reg;
  return reg;
}

template <typename MsgT, typename ColT>
RegistrarVrt<MsgT,ColT>::RegistrarVrt() {
  auto& reg = getTLRegistry();
  index = reg.size();
  reg.emplace_back(std::make_unique<DispatchCollection<ColT,MsgT>>());
}

template <typename MsgT, typename ColT>
inline AutoHandlerType registerVrtDispatch() {
  return RegistrarWrapperVrt<MsgT,ColT>().registrar.index;
}

template <typename MsgT, typename ColT>
AutoHandlerType const VrtDispatchHolder<MsgT,ColT>::idx =
  registerVrtDispatch<MsgT,ColT>();

inline DispatchBasePtrType getDispatch(AutoHandlerType const& han) {
  return getTLRegistry().at(han).get();
}

template <typename MsgT, typename ColT>
inline AutoHandlerType makeVrtDispatch(VirtualProxyType const& default_proxy) {
  auto const idx = VrtDispatchHolder<MsgT,ColT>::idx;
  if (default_proxy != no_vrt_proxy) {
    getDispatch(idx)->setDefaultProxy(default_proxy);
  }
  return idx;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DISPATCH_REGISTRY_IMPL_H*/
