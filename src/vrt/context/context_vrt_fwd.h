
#if !defined INCLUDED_CONTEXT_VRT_FWD
#define INCLUDED_CONTEXT_VRT_FWD

#include "config.h"
#include "messaging/message.h"
#include "context_vrt.h"

namespace vt { namespace vrt {

using VirtualIDType = uint32_t;
using VirtualRemoteIDType = uint16_t;
using VirtualRequestIDType = int64_t;

static constexpr VirtualRequestIDType const no_request_id = -1;

struct VirtualContextManager;

}}  // end namespace vt::vrt

#endif /*INCLUDED_CONTEXT_VRT_FWD*/

