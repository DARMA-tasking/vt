
#if !defined INCLUDED_COLLECTIVE_COLLECTIVE_OPS_H
#define INCLUDED_COLLECTIVE_COLLECTIVE_OPS_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/runtime/runtime_headers.h"
#include "vt/registry/registry.h"

#include <string>

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
  static void abort(std::string const str = "", ErrorCodeType const code = 1);
  static void output(
    std::string const str = "", ErrorCodeType const code = 1,
    bool error = false, bool decorate = true
  );

  static HandlerType registerHandler(ActiveClosureFnType fn);
};

using CollectiveOps = CollectiveAnyOps<collective_default_inst>;

// Export the default CollectiveOps::{abort,output} to the vt namespace
void abort(std::string const str = "", ErrorCodeType const code = 1);
void output(
  std::string const str = "", ErrorCodeType const code = 1, bool error = false,
  bool decorate = true
);

} //end namespace vt

#endif /*INCLUDED_COLLECTIVE_COLLECTIVE_OPS_H*/
