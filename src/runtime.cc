
#include <memory>

#include "context/context.h"
#include "registry/registry.h"
#include "messaging/active.h"
#include "event/event.h"
#include "termination/termination.h"
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

std::unique_ptr<Context> theContext = nullptr;
std::unique_ptr<Registry> theRegistry = nullptr;
std::unique_ptr<ActiveMessenger> theMsg = nullptr;
std::unique_ptr<event::AsyncEvent> theEvent = nullptr;
std::unique_ptr<term::TerminationDetector> theTerm = nullptr;
std::unique_ptr<barrier::Barrier> theBarrier = nullptr;
std::unique_ptr<pool::Pool> thePool = nullptr;
std::unique_ptr<rdma::RDMAManager> theRDMA = nullptr;
std::unique_ptr<param::Param> theParam = nullptr;
std::unique_ptr<seq::Sequencer> theSeq = nullptr;
std::unique_ptr<seq::SequencerVirtual> theVirtualSeq = nullptr;
std::unique_ptr<sched::Scheduler> theSched = nullptr;
std::unique_ptr<location::LocationManager> theLocMan = nullptr;
std::unique_ptr<vrt::VirtualContextManager> theVirtualManager;
std::unique_ptr<worker::WorkerGroup> theWorkerGrp = nullptr;

#if backend_check_enabled(trace_enabled)
  std::unique_ptr<trace::Trace> theTrace = nullptr
#endif

} //end namespace vt
