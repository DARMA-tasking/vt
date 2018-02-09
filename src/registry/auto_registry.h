
#if !defined INCLUDED_REGISTRY_AUTO_REGISTRY_H
#define INCLUDED_REGISTRY_AUTO_REGISTRY_H

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry_general.h"
#include "registry.h"

#include "trace/trace.h"
#include "utils/demangle/demangle.h"

#include "activefn/activefn.h"
#include "vrt/context/context_vrt.h"

#include <vector>
#include <memory>

namespace vt { namespace auto_registry {

AutoActiveType getAutoHandler(HandlerType const& handler);

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
HandlerType makeAutoHandler(MessageT* const msg);

template <typename T, T value>
HandlerType makeAutoHandler();

}} // end namespace vt::auto_registry

#include "auto_registry_impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_H*/
