
#if !defined INCLUDED_COLLECTIVE_COLLECTIVE_H
#define INCLUDED_COLLECTIVE_COLLECTIVE_H

#include "config.h"
#include "context/context.h"
#include "runtime/runtime_headers.h"
#include "registry/registry.h"

#include <mpi.h>

namespace vt {

using namespace ::vt::runtime;

static constexpr RuntimeInstType const collective_default_inst =
  RuntimeInstType::DefaultInstance;

template <RuntimeInstType instance = collective_default_inst>
struct CollectiveAnyOps {
  // The general methods that interact with the managed runtime holder
  static RuntimePtrType initialize(
    int argc, char** argv, WorkerCountType const num_workers = no_workers,
    bool is_interop = false, MPI_Comm* comm = nullptr
  );
  static void finalize(RuntimePtrType in_rt = nullptr);
  static void scheduleThenFinalize(
    RuntimePtrType in_rt = nullptr, WorkerCountType const workers = no_workers
  );
  static void setCurrentRuntimeTLS(RuntimeUnsafePtrType in_rt = nullptr);

  static HandlerType registerHandler(ActiveClosureFnType fn);
};

using CollectiveOps = CollectiveAnyOps<collective_default_inst>;

} //end namespace vt

#endif /*INCLUDED_COLLECTIVE_COLLECTIVE_H*/
