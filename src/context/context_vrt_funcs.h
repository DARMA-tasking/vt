
#if !defined __RUNTIME_TRANSPORT_CONTEXT_VRT_FUNCS__
#define __RUNTIME_TRANSPORT_CONTEXT_VRT_FUNCS__

#include "config.h"
#include "message.h"
#include "context_vrt.h"

namespace vt { namespace vrt {

using ActiveVrtFnPtrType = void(*)(vt::BaseMessage *, vt::vrt::VrtContext*);

template <typename MessageT, typename VirtualContextT>
using ActiveVrtTypedFnType = void(MessageT*, VirtualContextT*);

}} /* end namespace vt::context */

#endif /*__RUNTIME_TRANSPORT_CONTEXT_VRT_FUNCS__*/
