
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_VC__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_VC__

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry_general.h"
#include "registry.h"

#include "registry_function.h"
#include "context/context_vrt.h"
#include "context_vrt_fwd.h"

namespace vt { namespace auto_registry {

AutoActiveVCType getAutoHandlerVC(HandlerType const& handler);

template <
  typename VirtualContextT,
  typename MessageT,
  ActiveVCFunctionType<MessageT, VirtualContextT>* f
>
HandlerType makeAutoHandlerVC(MessageT* const msg);

}} // end namespace vt::auto_registry

#include "auto_registry_vc_impl.h"

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_VC__*/
