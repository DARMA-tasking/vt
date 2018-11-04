
#include "vt/config.h"
#include "vt/runtime/runtime.h"
#include "vt/runtime/runtime_inst.h"
#include "vt/utils/tls/tls.h"

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

#define IS_COMM_THREAD curRT->theContext.get()->getWorker() == worker_id_comm_thread
#define CUR_RT_SAFE (IS_COMM_THREAD ? curRT : no_rt)
#define CUR_RT_TS curRT
#define CUR_RT CUR_RT_SAFE

#define CHECK_THD                                                       \
  do {                                                                  \
    bool const check = IS_COMM_THREAD;                                  \
    std::string str("Only comm thread can access this component");      \
    assert(check && str.c_str());                                       \
    if (!check) { CUR_RT->abort(str, 29); }                             \
  } while (0);

// Thread-safe runtime components
ctx::Context*               theContext()        { return CUR_RT_TS->theContext.get();        }
pool::Pool*                 thePool()           { return CUR_RT_TS->thePool.get();           }
vrt::VirtualContextManager* theVirtualManager() { return CUR_RT_TS->theVirtualManager.get(); }
worker::WorkerGroupType*    theWorkerGrp()      { return CUR_RT_TS->theWorkerGrp.get();      }

// Non thread-safe runtime components
collective::CollectiveAlg*  theCollective()     { return CUR_RT->theCollective.get();     }
event::AsyncEvent*          theEvent()          { return CUR_RT->theEvent.get();          }
messaging::ActiveMessenger* theMsg()            { return CUR_RT->theMsg.get();            }
param::Param*               theParam()          { return CUR_RT->theParam.get();          }
rdma::RDMAManager*          theRDMA()           { return CUR_RT->theRDMA.get();           }
registry::Registry*         theRegistry()       { return CUR_RT->theRegistry.get();       }
sched::Scheduler*           theSched()          { return CUR_RT->theSched.get();          }
seq::Sequencer*             theSeq()            { return CUR_RT->theSeq.get();            }
seq::SequencerVirtual*      theVirtualSeq()     { return CUR_RT->theVirtualSeq.get();     }
term::TerminationDetector*  theTerm()           { return CUR_RT->theTerm.get();           }
location::LocationManager*  theLocMan()         { return CUR_RT->theLocMan.get();         }
CollectionManagerType*      theCollection()     { return CUR_RT->theCollection.get();     }
group::GroupManager*        theGroup()          { return CUR_RT->theGroup.get();          }
pipe::PipeManager*          theCB()             { return CUR_RT->theCB.get();             }

#if backend_check_enabled(trace_enabled)
trace::Trace*               theTrace()          { return CUR_RT->theTrace.get();          }
#endif

#undef CUR_RT
#undef CUR_RT_SAFE
#undef IS_COMM_THREAD

} /* end namespace vt */
