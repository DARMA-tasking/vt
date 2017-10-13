
#if !defined INCLUDED_CONTEXT_VRT_FUNCTIONS
#define INCLUDED_CONTEXT_VRT_FUNCTIONS

#include "config.h"
#include "messaging/message.h"
#include "context_vrt.h"

namespace vt { namespace vrt {

using ActiveVirtualFnPtrType = void (*)(vt::BaseMessage *,
                                        vt::vrt::VirtualContext *);

template <typename MessageT, typename VirtualContextT>
using ActiveVrtTypedFnType = void(MessageT *, VirtualContextT *);

}}  // end namespace vt::context

#endif  /*INCLUDED_CONTEXT_VRT_FUNCTIONS*/
