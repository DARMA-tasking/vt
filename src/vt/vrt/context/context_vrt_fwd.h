
#if !defined INCLUDED_CONTEXT_VRT_FWD
#define INCLUDED_CONTEXT_VRT_FWD

#include "vt/config.h"
#include "vt/vrt/context/context_vrt.h"
#include "vt/messaging/message.h"

#include <cstdlib>
#include <cstdint>

namespace vt { namespace vrt {

using VirtualIDType = uint32_t;
using VirtualRemoteIDType = uint16_t;
using VirtualRequestIDType = int64_t;

static constexpr VirtualRequestIDType const no_request_id = -1;

struct VirtualContextManager;

template <typename VrtContextT, typename... Args>
struct VirtualMakeClosure;

}}  // end namespace vt::vrt

#endif /*INCLUDED_CONTEXT_VRT_FWD*/

