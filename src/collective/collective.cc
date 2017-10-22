
#include "collective.h"
#include "runtime/runtime.h"
#include "runtime/runtime_inst.h"

#include <memory>
#include <mpi.h>

namespace vt {

/*static*/ void CollectiveOps::initialize(
  int argc, char** argv, WorkerCountType const num_workers,
  bool is_interop, MPI_Comm* comm
) {
  using vt::runtime::RuntimeInst;
  using vt::runtime::Runtime;
  using vt::runtime::eRuntimeInstance;

  RuntimeInst<eRuntimeInstance::DefaultInstance>::rt = std::make_unique<Runtime>(
    argc, argv, num_workers, is_interop, comm
  );

  ::vt::rt = RuntimeInst<eRuntimeInstance::DefaultInstance>::rt.get();
  RuntimeInst<eRuntimeInstance::DefaultInstance>::rt->initialize();
}

/*static*/ void CollectiveOps::finalize() {
  using vt::runtime::RuntimeInst;
  using vt::runtime::Runtime;
  using vt::runtime::eRuntimeInstance;

  RuntimeInst<eRuntimeInstance::DefaultInstance>::rt = nullptr;
  ::vt::rt = nullptr;
}

/*static*/ HandlerType CollectiveOps::registerHandler(ActiveClosureFnType fn) {
  return theRegistry()->registerActiveHandler(fn);
}

} //end namespace vt
