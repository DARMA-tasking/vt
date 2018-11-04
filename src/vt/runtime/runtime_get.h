
#if !defined INCLUDED_RUNTIME_GET_H
#define INCLUDED_RUNTIME_GET_H

#include "vt/config.h"

#include "vt/context/context.h"
#include "vt/registry/registry.h"
#include "vt/messaging/active.h"
#include "vt/event/event.h"
#include "vt/termination/term_headers.h"
#include "vt/collective/collective_alg.h"
#include "vt/pool/pool.h"
#include "vt/rdma/rdma.h"
#include "vt/parameterization/parameterization.h"
#include "vt/sequence/sequencer_headers.h"
#include "vt/trace/trace.h"
#include "vt/scheduler/scheduler.h"
#include "vt/topos/location/location_headers.h"
#include "vt/vrt/context/context_vrtmanager.h"
#include "vt/vrt/collection/collection_headers.h"
#include "vt/worker/worker_headers.h"
#include "vt/group/group_headers.h"
#include "vt/pipe/pipe_headers.h"

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
extern registry::Registry*         theRegistry();
extern sched::Scheduler*           theSched();
extern seq::Sequencer*             theSeq();
extern seq::SequencerVirtual*      theVirtualSeq();
extern term::TerminationDetector*  theTerm();
extern location::LocationManager*  theLocMan();
extern CollectionManagerType*      theCollection();
extern group::GroupManager*        theGroup();
extern pipe::PipeManager*          theCB();

#if backend_check_enabled(trace_enabled)
extern trace::Trace*               theTrace();
#endif

// namespace worker   { extern WorkerGroup*           getWorkerGrp();      }
// namespace seq      { extern Sequencer*             getSeq();            }
// namespace seq      { extern SequencerVirtual*      theVirtualSeq();     }

} /* end namespace vt */

#endif /*INCLUDED_RUNTIME_GET_H*/
