
#if !defined INCLUDED_VRT_CONTEXT_CONTEXT_VRT_FUNCS_H
#define INCLUDED_VRT_CONTEXT_CONTEXT_VRT_FUNCS_H

#include "vt/config.h"
#include "vt/vrt/context/context_vrt.h"
#include "vt/messaging/message.h"

namespace vt { namespace vrt {

using ActiveVirtualFnPtrType = void(*)(vt::BaseMessage *, vt::vrt::VirtualContext*);

template <typename MessageT, typename VirtualContextT>
using ActiveVrtTypedFnType = void(MessageT*, VirtualContextT*);

}} /* end namespace vt::context */

#endif /*INCLUDED_VRT_CONTEXT/CONTEXT_VRT_FUNCS_H*/
