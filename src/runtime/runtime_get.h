
#if !defined INCLUDED_RUNTIME_GET_H
#define INCLUDED_RUNTIME_GET_H

#include "config.h"

#include "context/context.h"
#include "registry/registry.h"
#include "messaging/active.h"
#include "event/event.h"
#include "termination/term_headers.h"
#include "barrier/barrier.h"
#include "pool/pool.h"
#include "rdma/rdma.h"
#include "parameterization/parameterization.h"
#include "sequence/sequencer_headers.h"
#include "trace/trace.h"
#include "scheduler/scheduler.h"
#include "topos/location/location.h"
#include "vrt/context/context_vrtmanager.h"
#include "worker/worker_headers.h"

namespace vt {

extern ActiveMessenger* theMsg();
extern Registry* getRegistry();

extern ctx::Context*               theContext();
extern barrier::Barrier*           theBarrier();
extern event::AsyncEvent*          theEvent();
extern param::Param*               theParam();
extern rdma::RDMAManager*          theRDMA();
extern sched::Scheduler*           theSched();
extern term::TerminationDetector*  theTerm();
extern location::LocationManager*  theLocMan();
extern vrt::VirtualContextManager* theVirtualManager();
extern pool::Pool*                 thePool();

#if backend_check_enabled(trace_enabled)
extern trace::Trace*               theTrace();
#endif

// namespace worker   { extern WorkerGroup*           getWorkerGrp();      }
// namespace seq      { extern Sequencer*             getSeq();            }
// namespace seq      { extern SequencerVirtual*      theVirtualSeq();     }

} /* end namespace vt */

#endif /*INCLUDED_RUNTIME_GET_H*/
