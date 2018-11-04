
#include "vt/collective/collective_ops.h"
#include "vt/runtime/runtime.h"
#include "vt/runtime/runtime_inst.h"
#include "vt/utils/tls/tls.h"

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

#pragma sst global rt
  RuntimeInst<instance>::rt = std::make_unique<Runtime>(
    argc, argv, num_workers, is_interop, comm
  );

#pragma sst global rt
  auto rt_ptr = RuntimeInst<instance>::rt.get();
  if (instance == RuntimeInstType::DefaultInstance) {
    // Set global variable for default instance for backward compatibility
    ::vt::rt = rt_ptr;
    curRT = rt_ptr;
  }
#pragma sst global rt
  RuntimeInst<instance>::rt->initialize();

  return runtime::makeRuntimePtr(rt_ptr);
}

template <RuntimeInstType instance>
void CollectiveAnyOps<instance>::setCurrentRuntimeTLS(RuntimeUnsafePtrType in) {
  bool const has_rt = in != nullptr;
  auto rt_use = has_rt ? in : ::vt::rt;
  curRT = rt_use;
}

template <RuntimeInstType instance>
void CollectiveAnyOps<instance>::scheduleThenFinalize(
  RuntimePtrType in_rt, WorkerCountType const workers
) {
  bool const has_rt = in_rt != nullptr;
  auto rt_use = has_rt ? in_rt.unsafe() : curRT;

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

#pragma sst global rt
  RuntimeInst<instance>::rt = nullptr;

  if (instance == RuntimeInstType::DefaultInstance) {
    // Set global variable for default instance for backward compatibility
    ::vt::rt = nullptr;
    curRT = nullptr;
  }

  if (in_rt) {
    in_rt = nullptr;
  }
}

template <RuntimeInstType instance>
void CollectiveAnyOps<instance>::abort(
  std::string const str, ErrorCodeType const code
) {
  auto tls_rt = curRT;
  auto rt = tls_rt ? tls_rt : ::vt::rt;
  if (rt) {
    rt->abort(str, code);
  } else {
    std::exit(code);
  }
}

template <RuntimeInstType instance>
void CollectiveAnyOps<instance>::output(
  std::string const str, ErrorCodeType const code, bool error, bool decorate
) {
  auto tls_rt = curRT;
  auto rt = tls_rt ? tls_rt : ::vt::rt;
  if (rt) {
    rt->output(str,code,error,decorate);
  } else {
    ::fmt::print(str.c_str());
  }
}

template <RuntimeInstType instance>
HandlerType CollectiveAnyOps<instance>::registerHandler(ActiveClosureFnType fn) {
  return theRegistry()->registerActiveHandler(fn);
}

template struct CollectiveAnyOps<collective_default_inst>;

void abort(std::string const str, ErrorCodeType const code) {
  return CollectiveOps::abort(str,code);
}

void output(
  std::string const str, ErrorCodeType const code, bool error, bool decorate
) {
  return CollectiveOps::output(str,code,error,decorate);
}

} //end namespace vt
