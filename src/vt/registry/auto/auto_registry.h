
#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_H

#include "vt/config.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/registry/registry.h"

#include "vt/trace/trace.h"
#include "vt/utils/demangle/demangle.h"

#include "vt/activefn/activefn.h"
#include "vt/vrt/context/context_vrt.h"

#include <vector>
#include <memory>

namespace vt { namespace auto_registry {

AutoActiveType getAutoHandler(HandlerType const& handler);

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
HandlerType makeAutoHandler(MessageT* const msg);

template <typename T, T value>
HandlerType makeAutoHandler();

}} // end namespace vt::auto_registry

#include "vt/registry/auto/auto_registry_impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_H*/
