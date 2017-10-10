
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_VC__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_VC__

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry_general.h"
#include "registry.h"
#include "activefn/activefn.h"
#include "vrt/context/context_vrt_funcs.h"

namespace vt { namespace auto_registry {

using namespace vrt;

AutoActiveVCType getAutoHandlerVC(HandlerType const& handler);

template <typename VrtT, typename MsgT, ActiveVrtTypedFnType<MsgT, VrtT>* f>
HandlerType makeAutoHandlerVC(MsgT* const msg);

}} // end namespace vt::auto_registry

#include "auto_registry_vc_impl.h"

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_VC__*/
