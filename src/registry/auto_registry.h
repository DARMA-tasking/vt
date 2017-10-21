
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

// convenience macro for registration
#define GET_HANDLER_ACTIVE_FUNCTION_EXPAND(TYPE_F, ADD_F)               \
  vt::auto_registry::RunnableGen<                                       \
   decltype(vt::auto_registry::FunctorAdapter<TYPE_F, ADD_F>()),        \
   AutoActiveContainerType, AutoRegInfoType<AutoActiveType>,            \
   ActiveFnPtrType                                                         \
  >::idx;

#include "auto_registry_impl.h"

#endif /*INCLUDED_REGISTRY_AUTO_REGISTRY_H*/
