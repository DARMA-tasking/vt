
#if !defined INCLUDED_RUNTIME_GET_H
#define INCLUDED_RUNTIME_GET_H

#include "config.h"

#include "context/context.h"
#include "registry/registry.h"
#include "messaging/active.h"
#include "event/event.h"
#include "termination/term_headers.h"
#include "collective/collective_alg.h"
#include "pool/pool.h"
#include "rdma/rdma.h"
#include "parameterization/parameterization.h"
#include "sequence/sequencer_headers.h"
#include "trace/trace.h"
#include "scheduler/scheduler.h"
#include "topos/location/location_headers.h"
#include "vrt/context/context_vrtmanager.h"
#include "vrt/collection/collection_headers.h"
#include "worker/worker_headers.h"
#include "group/group_headers.h"

namespace vt {

using CollectionManagerType = vrt::collection::CollectionManager;

extern ctx::Context*               theContext();
extern pool::Pool*                 thePool();
extern vrt::VirtualContextManager* theVirtualManager();
extern worker::WorkerGroupType*    theWorkerGrp();
extern collective::CollectiveAlg*  theCollective();
extern event::AsyncEvent*          theEvent();
extern messaging::ActiveMessenger* theMsg();
extern param::Param*               theParam();
extern rdma::RDMAManager*          theRDMA();
extern Registry*                   theRegistry();
extern sched::Scheduler*           theSched();
extern seq::Sequencer*             theSeq();
extern seq::SequencerVirtual*      theVirtualSeq();
extern term::TerminationDetector*  theTerm();
extern location::LocationManager*  theLocMan();
extern CollectionManagerType*      theCollection();
extern group::GroupManager*        theGroup();

#if backend_check_enabled(trace_enabled)
extern trace::Trace*               theTrace();
#endif

// namespace worker   { extern WorkerGroup*           getWorkerGrp();      }
// namespace seq      { extern Sequencer*             getSeq();            }
// namespace seq      { extern SequencerVirtual*      theVirtualSeq();     }

} /* end namespace vt */

#endif /*INCLUDED_RUNTIME_GET_H*/
