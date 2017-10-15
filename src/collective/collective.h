
#if ! defined __RUNTIME_TRANSPORT_COLLECTIVE__
#define __RUNTIME_TRANSPORT_COLLECTIVE__

#include "config.h"
#include "context/context.h"
#include "registry/registry.h"

#include <mpi.h>

namespace vt {

extern bool vtIsWorking;

struct CollectiveOps {
  static void initialize(
    int argc, char** argv, WorkerCountType const num_workers = no_workers,
    bool is_interop = false, MPI_Comm* comm = nullptr
  );
  static void finalize();

  static void initializeContext(
    int argc, char** argv, bool is_interop = false, MPI_Comm* comm = nullptr
  );
  static void initializeRuntime();
  static void initializeComponents();
  static void initializeWorkers(WorkerCountType const num_workers = no_workers);
  static void finalizeContext(bool is_interop = false);
  static void finalizeRuntime();
  static void finalizeComponents();
  static void finalizeWorkers();

  static void setInactiveState();
  static HandlerType registerHandler(ActiveClosureFnType fn);
};

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_COLLECTIVE__*/
