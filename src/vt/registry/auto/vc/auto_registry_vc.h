
#if !defined INCLUDED_REGISTRY_AUTO_VC_AUTO_REGISTRY_VC_H
#define INCLUDED_REGISTRY_AUTO_VC_AUTO_REGISTRY_VC_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/registry/registry.h"
#include "vt/activefn/activefn.h"
#include "vt/vrt/context/context_vrt_funcs.h"

namespace vt { namespace auto_registry {

using namespace vrt;

AutoActiveVCType getAutoHandlerVC(HandlerType const& handler);

template <typename VrtT, typename MsgT, ActiveVrtTypedFnType<MsgT, VrtT>* f>
HandlerType makeAutoHandlerVC(MsgT* const msg);

}} // end namespace vt::auto_registry

#include "vt/registry/auto/vc/auto_registry_vc_impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_VC_AUTO_REGISTRY_VC_H*/
