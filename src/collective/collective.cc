
#include "collective.h"
#include "runtime/runtime.h"
#include "runtime/runtime_inst.h"
#include "utils/tls/tls.h"

#include <memory>
#include <cstdlib>
#include <mpi.h>

namespace vt {

using namespace ::vt::runtime;

template <RuntimeInstType instance>
RuntimePtrType CollectiveAnyOps<instance>::initialize(
  int argc, char** argv, WorkerCountType const num_workers,
  bool is_interop, MPI_Comm* comm
) {
  using vt::runtime::RuntimeInst;
  using vt::runtime::Runtime;
  using vt::runtime::eRuntimeInstance;

  RuntimeInst<instance>::rt = std::make_unique<Runtime>(
    argc, argv, num_workers, is_interop, comm
  );

  auto rt_ptr = RuntimeInst<instance>::rt.get();
  if (instance == RuntimeInstType::DefaultInstance) {
    // Set global variable for default instance for backward compatibility
    ::vt::rt = rt_ptr;
    AccessTLS(curRT) = rt_ptr;
  }
  RuntimeInst<instance>::rt->initialize();

  return runtime::makeRuntimePtr(rt_ptr);
}

template <RuntimeInstType instance>
void CollectiveAnyOps<instance>::setCurrentRuntimeTLS(RuntimeUnsafePtrType in) {
  bool const has_rt = in != nullptr;
  auto rt_use = has_rt ? in : ::vt::rt;
  AccessTLS(curRT) = rt_use;
}

template <RuntimeInstType instance>
void CollectiveAnyOps<instance>::scheduleThenFinalize(
  RuntimePtrType in_rt, WorkerCountType const workers
) {
  bool const has_rt = in_rt != nullptr;
  auto rt_use = has_rt ? in_rt.unsafe() : AccessTLS(curRT);

  auto sched_fn = [=]{
    while (not rt_use->isTerminated()) {
      runScheduler();
    }
  };

  if (workers == no_workers) {
    sched_fn();
  } else {
    theWorkerGrp()->spawnWorkersBlock(sched_fn);
  }

  CollectiveAnyOps<instance>::finalize(has_rt ? std::move(in_rt) : nullptr);
}

template <RuntimeInstType instance>
void CollectiveAnyOps<instance>::finalize(RuntimePtrType in_rt) {
  using vt::runtime::RuntimeInst;
  using vt::runtime::Runtime;
  using vt::runtime::eRuntimeInstance;

  RuntimeInst<instance>::rt = nullptr;

  if (instance == RuntimeInstType::DefaultInstance) {
    // Set global variable for default instance for backward compatibility
    ::vt::rt = nullptr;
    AccessTLS(curRT) = nullptr;
  }

  if (in_rt) {
    in_rt = nullptr;
  }
}

template <RuntimeInstType instance>
void CollectiveAnyOps<instance>::abort(
  std::string const str, ErrorCodeType const code
) {
  auto tls_rt = AccessTLS(curRT);
  auto rt = tls_rt ? tls_rt : ::vt::rt;
  if (rt) {
    rt->abort(str, code);
  } else {
    std::exit(code);
  }
}

template <RuntimeInstType instance>
HandlerType CollectiveAnyOps<instance>::registerHandler(ActiveClosureFnType fn) {
  return theRegistry()->registerActiveHandler(fn);
}

template struct CollectiveAnyOps<collective_default_inst>;

} //end namespace vt
