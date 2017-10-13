
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY__

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

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY__*/
