
#include "config.h"
#include "runtime.h"

namespace vt {

ctx::Context*               theContext()        { return rt->theContext.get();        }
barrier::Barrier*           theBarrier()        { return rt->theBarrier.get();        }
event::AsyncEvent*          theEvent()          { return rt->theEvent.get();          }
ActiveMessenger*            theMsg()            { return rt->theMsg.get();            }
param::Param*               theParam()          { return rt->theParam.get();          }
rdma::RDMAManager*          theRDMA()           { return rt->theRDMA.get();           }
Registry*                   theRegistry()       { return rt->theRegistry.get();       }
sched::Scheduler*           theSched()          { return rt->theSched.get();          }
seq::Sequencer*             theSeq()            { return rt->theSeq.get();            }
seq::SequencerVirtual*      theVirtualSeq()     { return rt->theVirtualSeq.get();     }
term::TerminationDetector*  theTerm()           { return rt->theTerm.get();           }
location::LocationManager*  theLocMan()         { return rt->theLocMan.get();         }
vrt::VirtualContextManager* theVirtualManager() { return rt->theVirtualManager.get(); }
worker::WorkerGroup*        theWorkerGrp()      { return rt->theWorkerGrp.get();      }
pool::Pool*                 thePool()           { return rt->thePool.get();           }

#if backend_check_enabled(trace_enabled)
trace::Trace*               theTrace()          { return rt->theTrace.get();          }
#endif

} /* end namespace vt */
