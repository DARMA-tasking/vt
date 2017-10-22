
#if !defined INCLUDED_RUNTIME_COMPONENT_FWD_H
#define INCLUDED_RUNTIME_COMPONENT_FWD_H

#include "config.h"

namespace vt {

struct ActiveMessenger;
struct Registry;

namespace ctx      { struct Context;               } /* end namespace ctx.     */
namespace barrier  { struct Barrier;               } /* end namespace barrier  */
namespace tx       { struct Context;               } /* end namespace tx       */
namespace event    { struct AsyncEvent;            } /* end namespace event    */
namespace param    { struct Param;                 } /* end namespace param    */
namespace pool     { struct Pool;                  } /* end namespace pool     */
namespace rdma     { struct RDMAManager;           } /* end namespace rdma     */
namespace sched    { struct Scheduler;             } /* end namespace sched    */
namespace term     { struct TermWorker;            } /* end namespace term     */
namespace term     { struct TerminationDetector;   } /* end namespace term     */
namespace location { struct LocationManager;       } /* end namespace location */
namespace trace    { struct Trace;                 } /* end namespace trace    */
namespace vrt      { struct VirtualContextManager; } /* end namespace vrt      */

} /* end namespace vt */

#include "sequence/sequencer_fwd.h"
#include "worker/worker_group_fwd.h"

#endif /*INCLUDED_RUNTIME_COMPONENT_FWD_H*/
