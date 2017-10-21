
#if !defined INCLUDED_VRT_CONTEXT_CONTEXT_VRT_FUNCS_H
#define INCLUDED_VRT_CONTEXT_CONTEXT_VRT_FUNCS_H

#include "config.h"
#include "messaging/message.h"
#include "context_vrt.h"

namespace vt { namespace vrt {

using ActiveVirtualFnPtrType = void(*)(vt::BaseMessage *, vt::vrt::VirtualContext*);

template <typename MessageT, typename VirtualContextT>
using ActiveVrtTypedFnType = void(MessageT*, VirtualContextT*);

}} /* end namespace vt::context */

#endif /*INCLUDED_VRT_CONTEXT/CONTEXT_VRT_FUNCS_H*/
