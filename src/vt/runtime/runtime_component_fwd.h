
#if !defined INCLUDED_VT_RUNTIME_RUNTIME_COMPONENT_FWD_H
#define INCLUDED_VT_RUNTIME_RUNTIME_COMPONENT_FWD_H

#include "vt/config.h"
#include "vt/termination/term_common.h"
#include "vt/sequence/sequencer_fwd.h"

namespace vt {

namespace registry {
struct Registry;
}
namespace messaging {
struct ActiveMessenger;
}
namespace ctx {
struct Context;
}
namespace event {
struct AsyncEvent;
}
namespace collective {
struct CollectiveAlg;
}
namespace pool {
struct Pool;
}
namespace rdma {
struct RDMAManager;
}
namespace param {
struct Param;
}
namespace sched {
struct Scheduler;
}
namespace location {
struct LocationManager;
}
namespace vrt {
struct VirtualContextManager;
}
namespace vrt { namespace collection {
struct CollectionManager;
}}
namespace group {
struct GroupManager;
}
namespace pipe {
struct PipeManager;
}
namespace fetch {
struct FetchManager;
}

#if backend_check_enabled(trace_enabled)
namespace trace {
struct Trace;
}
#endif

} /* end namespace vt */

#endif /*INCLUDED_VT_RUNTIME_RUNTIME_COMPONENT_FWD_H*/
