
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

namespace runtime {

std::unique_ptr<Context> the_context = nullptr;
std::unique_ptr<Registry> the_registry = std::make_unique<Registry>();
std::unique_ptr<ActiveMessenger> the_msg = std::make_unique<ActiveMessenger>();
std::unique_ptr<AsyncEvent> the_event = std::make_unique<AsyncEvent>();
std::unique_ptr<term::TerminationDetector> the_term = std::make_unique<term::TerminationDetector>();
std::unique_ptr<barrier::Barrier> the_barrier = std::make_unique<barrier::Barrier>();
std::unique_ptr<pool::Pool> the_pool = std::make_unique<pool::Pool>();
std::unique_ptr<rdma::RDMAManager> the_rdma = std::make_unique<rdma::RDMAManager>();
std::unique_ptr<param::Param> the_param = std::make_unique<param::Param>();
std::unique_ptr<seq::Sequencer> the_seq = std::make_unique<seq::Sequencer>();

backend_enable_if(
  trace_enabled,
  std::unique_ptr<trace::Trace> the_trace = std::make_unique<trace::Trace>();
);

} //end namespace runtime
