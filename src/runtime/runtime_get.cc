
#include "config.h"
#include "runtime.h"
#include "runtime_inst.h"
#include "utils/tls/tls.h"

#include <cassert>

namespace vt {

/*
 *         === Access functions for the runtime component singletons ===
 *
 * Macro is used for convenience so these can be changed easily.
 *
 * Currently, there is only one runtime instance that is actually used, which is
 * accessed through the TLS variable `curRT'. Using a TLS variable enables
 * switching the pointer for other non-communication threads that should not
 * have directly access to runtime components that are not thread-safe.
 *
 * The use of a TLS variable for every access to the singletons might slow down
 * performance. If so, we may need to redesign this.
 *
 */

static runtime::Runtime* no_rt = nullptr;

#define CUR_RT curRT
#define IS_COMM_THREAD ::vt::theContext()->getWorker() == worker_id_comm_thread
#define CUR_RT_SAFE (IS_COMM_THREAD ? curRT : no_rt)

#define CHECK_THD                                                       \
  do {                                                                  \
    bool const check = IS_COMM_THREAD;                                  \
    std::string str("Only comm thread can access this component");      \
    assert(check && str.c_str());                                       \
    if (!check) { CUR_RT->abort(str, 29); }                             \
  } while (0);

ctx::Context*               theContext()        { return CUR_RT->theContext.get();        }
barrier::Barrier*           theBarrier()        { return CUR_RT->theBarrier.get();       }
collective::CollectiveAlg*  theCollective()     { return CUR_RT->theCollective.get();        }
event::AsyncEvent*          theEvent()          { return CUR_RT->theEvent.get();          }
ActiveMessenger*            theMsg()            { return CUR_RT->theMsg.get();            }
param::Param*               theParam()          { return CUR_RT->theParam.get();          }
rdma::RDMAManager*          theRDMA()           { return CUR_RT->theRDMA.get();           }
Registry*                   theRegistry()       { return CUR_RT->theRegistry.get();       }
sched::Scheduler*           theSched()          { return CUR_RT->theSched.get();          }
seq::Sequencer*             theSeq()            { return CUR_RT->theSeq.get();            }
seq::SequencerVirtual*      theVirtualSeq()     { return CUR_RT->theVirtualSeq.get();     }
term::TerminationDetector*  theTerm()           { return CUR_RT->theTerm.get();           }
location::LocationManager*  theLocMan()         { return CUR_RT->theLocMan.get();         }
vrt::VirtualContextManager* theVirtualManager() { return CUR_RT->theVirtualManager.get(); }
CollectionManagerType*      theCollection()     { return CUR_RT->theCollection.get();     }
worker::WorkerGroupType*    theWorkerGrp()      { return CUR_RT->theWorkerGrp.get();      }
pool::Pool*                 thePool()           { return CUR_RT->thePool.get();           }

#if backend_check_enabled(trace_enabled)
trace::Trace*               theTrace()          { return CUR_RT->theTrace.get();          }
#endif

#undef CUR_RT
#undef CUR_RT_SAFE
#undef IS_COMM_THREAD

} /* end namespace vt */
