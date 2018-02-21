
#include "config.h"
#include "runtime.h"

#include "context/context.h"
#include "context/context_attorney.h"
#include "registry/registry.h"
#include "messaging/active.h"
#include "event/event.h"
#include "termination/termination.h"
#include "pool/pool.h"
#include "rdma/rdma_headers.h"
#include "parameterization/parameterization.h"
#include "sequence/sequencer_headers.h"
#include "trace/trace.h"
#include "scheduler/scheduler.h"
#include "topos/location/location_headers.h"
#include "vrt/context/context_vrtmanager.h"
#include "vrt/collection/collection_headers.h"
#include "worker/worker_headers.h"

#include <memory>
#include <functional>
#include <string>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

namespace vt { namespace runtime {

Runtime::Runtime(
  int argc, char** argv, WorkerCountType in_num_workers,
  bool const interop_mode, MPI_Comm* in_comm, RuntimeInstType const in_instance
)  : instance_(in_instance), runtime_active_(false), is_interop_(interop_mode),
     num_workers_(in_num_workers), communicator_(in_comm), user_argc_(argc),
     user_argv_(argv)
{ }

/*virtual*/ Runtime::~Runtime() {
  while (runtime_active_ && !aborted_) {
    runScheduler();
  }
  if (!aborted_) {
    finalize();
  }
}

bool Runtime::tryInitialize() {
  bool const init_now = !initialized_ && !finalized_;

  debug_print(
    runtime, unknown,
    "Runtime: tryInitialize: initialized_=%s, finalized_=%s, init_now=%s\n",
    print_bool(initialized_), print_bool(finalized_), print_bool(init_now)
  );

  if (init_now) {
    initialize(true);
  }
  return init_now;
}

bool Runtime::tryFinalize() {
  bool const rt_live = !finalized_ && initialized_;
  bool const has_run_sched = hasSchedRun();
  bool const finalize_now = rt_live && has_run_sched;

  debug_print(
    runtime, unknown,
    "Runtime: tryFinalize: initialized_=%s, finalized_=%s, rt_live=%s, sched=%s, "
    "finalize_now=%s\n",
    print_bool(initialized_), print_bool(finalized_), print_bool(rt_live),
    print_bool(has_run_sched), print_bool(finalize_now)
  );

  if (finalize_now) {
    finalize(true);
  } else {
    finalize_on_term_ = true;
  }

  return finalize_now;
}

void Runtime::printStartupBanner() {
  NodeType const nodes = theContext->getNumNodes();
  WorkerCountType const workers = theContext->getNumWorkers();
  bool const has_workers = theContext->hasWorkers();

  std::string init = "Runtime initializing";
  std::string mode = std::string("mode: ") +
    std::string(num_workers_ == no_workers ? "single" : "multi") +
    std::string("-threaded");
  std::string thd = !has_workers ? std::string("") :
    std::string(", worker threading: ") +
    std::string(
      backend_enable_if_else(
        openmp, "OpenMP", backend_enable_if_else(stdthread, "std::thread", "")
      )
   );
  std::string worker_cnt = !has_workers ? std::string("") :
    (std::string(", ") + std::to_string(workers) + std::string(" workers/node"));

  auto const& stream = stdout;
  fprintf(stream, "VT: %s: %s%s\n", init.c_str(), mode.c_str(), thd.c_str());
  fprintf(stream, "VT: Running on %d nodes%s\n", nodes, worker_cnt.c_str());
  fprintf(stream, "VT: features enabled:\n" backend_print_all_features(0));
}

void Runtime::printShutdownBanner(term::TermCounterType const& num_units) {
  std::string fin = "Runtime finalizing";
  std::string units = std::to_string(num_units);
  auto const& stream = stdout;
  fprintf(stream, "VT: %s: %s work units processed\n", fin.c_str(), units.c_str());
}

bool Runtime::initialize(bool const force_now) {
  if (force_now) {
    initializeContext(user_argc_, user_argv_, communicator_);
    initializeComponents();
    initializeOptionalComponents();
    sync();
    if (theContext->getNode() == 0) {
      printStartupBanner();
    }
    setup();
    sync();
    initialized_ = true;
    return true;
  } else {
    return tryInitialize();
  }
}

bool Runtime::finalize(bool const force_now) {
  if (force_now) {
    auto const& is_zero = theContext->getNode() == 0;
    auto const& num_units = theTerm->getNumUnits();
    sync();
    fflush(stdout);
    fflush(stderr);
    sync();
    finalizeComponents();
    finalizeOptionalComponents();
    sync();
    sync();
    if (is_zero) {
      printShutdownBanner(num_units);
    }
    finalizeContext();
    finalized_ = true;
    return true;
  } else {
    return tryFinalize();
  }
}

void Runtime::sync() {
  MPI_Barrier(theContext->getComm());
}

void Runtime::runScheduler() {
  theSched->scheduler();
}

void Runtime::abort(std::string const abort_str, ErrorCodeType const code) {
  aborted_ = true;

  NodeType const node = theContext ? theContext->getNode() : -1;
  std::string sep = "--------------";
  auto csep = sep.c_str();
  fprintf(stderr, "VT: runtime error: system aborting\n");
  fprintf(stderr, "%s Node %d Exiting: abort() invoked %s\n", csep, node, csep);
  fprintf(stderr, "Error code: %d\n", code);
  fprintf(stderr, "Reason: \"%s\"\n", abort_str.c_str());

  std::exit(code);
}

void Runtime::terminationHandler() {
  debug_print(
    runtime, node,
    "Runtime: executing registered termination handler\n",
  );

  runtime_active_ = false;

  if (finalize_on_term_) {
    finalize();
  }
}

void Runtime::setup() {
  debug_print(runtime, node, "begin: setup\n");

  runtime_active_ = true;

  auto handler = std::bind(&Runtime::terminationHandler, this);
  term::TerminationDetector::registerDefaultTerminationAction(handler);

  MPI_Barrier(theContext->getComm());

  // wait for all nodes to start up to initialize the runtime
  theCollective->barrierThen([this]{
    MPI_Barrier(theContext->getComm());
  });

  backend_enable_if(
    trace_enabled, {
      std::string name = argc == 0 ? "prog" : argv[0];
      auto const& node = theContext->getNode();
      theTrace->setupNames(name, name + "." + std::to_string(node) + ".log.gz");
    }
  );

  debug_print(runtime, node, "end: setup\n");
}

void Runtime::initializeContext(int argc, char** argv, MPI_Comm* comm) {
  theContext = std::make_unique<ctx::Context>(argc, argv, is_interop_, comm);

  debug_print(runtime, node, "finished initializing context\n");
}

void Runtime::finalizeContext() {
  MPI_Barrier(theContext->getComm());

  if (not is_interop_) {
    MPI_Finalize();
  }
}

void Runtime::initializeComponents() {
  debug_print(runtime, node, "begin: initializeComponents\n");

  // Helper components: not allowed to send messages during construction
  theRegistry = std::make_unique<Registry>();
  theEvent = std::make_unique<event::AsyncEvent>();
  thePool = std::make_unique<pool::Pool>();

  // Core components: enables more complex subsequent initialization
  theMsg = std::make_unique<messaging::ActiveMessenger>();
  theSched = std::make_unique<sched::Scheduler>();
  theTerm = std::make_unique<term::TerminationDetector>();
  theCollective = std::make_unique<collective::CollectiveAlg>();
  theGroup = std::make_unique<group::GroupManager>();

  // Advanced runtime components: not required for basic messaging
  theRDMA = std::make_unique<rdma::RDMAManager>();
  theParam = std::make_unique<param::Param>();
  theSeq = std::make_unique<seq::Sequencer>();
  theVirtualSeq = std::make_unique<seq::SequencerVirtual>();
  theLocMan = std::make_unique<location::LocationManager>();
  theVirtualManager = std::make_unique<vrt::VirtualContextManager>();
  theCollection = std::make_unique<vrt::collection::CollectionManager>();

  debug_print(runtime, node, "end: initializeComponents\n");
}

void Runtime::initializeOptionalComponents() {
  debug_print(runtime, node, "begin: initializeOptionalComponents\n");

  backend_enable_if(
    trace_enabled,
    theTrace = std::make_unique<trace::Trace>();
  );

  initializeWorkers(num_workers_);

  debug_print(runtime, node, "end: initializeOptionalComponents\n");
}

void Runtime::initializeWorkers(WorkerCountType const num_workers) {
  using ::vt::ctx::ContextAttorney;

  debug_print(
    runtime, node, "begin: initializeWorkers: workers=%d\n", num_workers
  );

  bool const has_workers = num_workers != no_workers;

  if (has_workers) {
    ContextAttorney::setNumWorkers(num_workers);

    // Initialize individual memory pool for each worker
    thePool->initWorkerPools(num_workers);

    theWorkerGrp = std::make_unique<worker::WorkerGroupType>();

    auto localTermFn = [](worker::eWorkerGroupEvent event){
      bool const no_local_workers = false;
      bool const is_idle = event == worker::eWorkerGroupEvent::WorkersIdle;
      bool const is_busy = event == worker::eWorkerGroupEvent::WorkersBusy;
      if (is_idle || is_busy) {
        ::vt::theTerm()->setLocalTerminated(is_idle, no_local_workers);
      }
    };
    theWorkerGrp->registerIdleListener(localTermFn);
  } else {
    // Without workers running on the node, the termination detector should
    // assume its locally ready to propagate instead of waiting for them to
    // become idle.
    theTerm->setLocalTerminated(true);
  }

  debug_print(runtime, node, "end: initializeWorkers\n");
}

void Runtime::finalizeComponents() {
  debug_print(runtime, node, "begin: finalizeComponents\n");

  // Reverse order destruction of runtime components.

  // Advanced components: may communicate during destruction
  theVirtualManager = nullptr;
  theLocMan = nullptr;
  theVirtualSeq = nullptr;
  theSeq = nullptr;
  theParam = nullptr;
  theRDMA = nullptr;

  // Core components
  theCollective = nullptr;
  theTerm = nullptr;
  theSched = nullptr;
  theMsg = nullptr;
  theGroup = nullptr;

  // Helper components: thePool the last to be destructed because it handles
  // memory allocations
  theRegistry = nullptr;
  theEvent->cleanup(); theEvent = nullptr;

  // Initialize individual memory pool for each worker
  thePool->destroyWorkerPools();
  thePool = nullptr;

  debug_print(runtime, node, "end: finalizeComponents\n");
}

void Runtime::finalizeOptionalComponents() {
  debug_print(runtime, node, "begin: finalizeOptionalComponents\n");

  theWorkerGrp = nullptr;

  debug_print(runtime, node, "end: finalizeOptionalComponents\n");
}

}} //end namespace vt::runtime
