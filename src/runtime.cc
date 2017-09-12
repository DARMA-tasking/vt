
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
#include "sequencer.h"
#include "trace.h"
#include "scheduler.h"

namespace vt {

std::unique_ptr<Context> theContext = nullptr;
std::unique_ptr<Registry> theRegistry = std::make_unique<Registry>();
std::unique_ptr<ActiveMessenger> theMsg = std::make_unique<ActiveMessenger>();
std::unique_ptr<AsyncEvent> theEvent = std::make_unique<AsyncEvent>();
std::unique_ptr<term::TerminationDetector> theTerm = std::make_unique<term::TerminationDetector>();
std::unique_ptr<barrier::Barrier> theBarrier = std::make_unique<barrier::Barrier>();
std::unique_ptr<pool::Pool> thePool = std::make_unique<pool::Pool>();
std::unique_ptr<rdma::RDMAManager> theRDMA = std::make_unique<rdma::RDMAManager>();
std::unique_ptr<param::Param> theParam = std::make_unique<param::Param>();
std::unique_ptr<seq::Sequencer> theSeq = std::make_unique<seq::Sequencer>();
std::unique_ptr<sched::Scheduler> theSched = std::make_unique<sched::Scheduler>();

backend_enable_if(
  trace_enabled,
  std::unique_ptr<trace::Trace> theTrace = std::make_unique<trace::Trace>();
);

} //end namespace vt
