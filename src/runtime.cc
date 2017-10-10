
#include <memory>

#include "context.h"
#include "registry.h"
#include "active.h"
#include "event.h"
#include "termination.h"
#include "barrier.h"
#include "pool.h"
#include "rdma.h"
#include "parameterization.h"
#include "sequence/sequencer_headers.h"
#include "trace.h"
#include "scheduler.h"
#include "location.h"
#include "vrt/context/context_vrtmanager.h"
#include "worker_headers.h"

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
