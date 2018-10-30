
#if !defined INCLUDED_VRT_COLLECTION_DISPATCH_REGISTRY_H
#define INCLUDED_VRT_COLLECTION_DISPATCH_REGISTRY_H

#include "config.h"
#include "registry/auto/auto_registry_common.h"
#include "vrt/collection/dispatch/dispatch.h"

#include <vector>
#include <memory>

namespace vt { namespace vrt { namespace collection {

using AutoHandlerType           = auto_registry::AutoHandlerType;
using DispatchBaseType          = DispatchCollectionBase;
using DispatchBasePtrType       = DispatchCollectionBase*;
using DispatchBaseOwningPtrType = std::unique_ptr<DispatchCollectionBase>;
using RegistryTLType            = std::vector<DispatchBaseOwningPtrType>;

inline RegistryTLType& getTLRegistry();

template <typename MsgT, typename ColT>
struct RegistrarVrt {
  RegistrarVrt();
  AutoHandlerType index;
};

template <typename MsgT, typename ColT>
struct RegistrarWrapperVrt {
  RegistrarVrt<MsgT,ColT> registrar;
};

template <typename MsgT, typename ColT>
struct VrtDispatchHolder {
  static AutoHandlerType const idx;
};

template <typename MsgT, typename ColT>
inline AutoHandlerType registerVrtDispatch();

inline DispatchBasePtrType getDispatch(AutoHandlerType const& han);

template <typename MsgT, typename ColT>
inline AutoHandlerType makeVrtDispatch(
  VirtualProxyType const& default_proxy = no_vrt_proxy
);

}}} /* end namespace vt::vrt::collection: */

#endif /*INCLUDED_VRT_COLLECTION_DISPATCH_REGISTRY_H*/
