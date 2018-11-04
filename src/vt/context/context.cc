
#include "vt/context/context.h"

#include <string>
#include <cstring>

#include <mpi.h>

// This cannot use the normal debug_print macros because they rely on context
// being live to print contextual information
#define DEBUG_VT_CONTEXT 0

namespace vt { namespace ctx {

Context::Context(int argc, char** argv, bool const is_interop, MPI_Comm* comm) {
  #if DEBUG_VT_CONTEXT
    fmt::print(
      "Context::Context is_interop={}, comm={}\n", print_bool(is_interop), comm
    );
  #endif

  if (argc > 0 && argv[0] != nullptr) {
    auto const app_name = std::string(argv[0]);
    auto const default_lb = LBType::HierarchicalLB;
    if (app_name.size() >= 7) {
      auto const app_sub = app_name.substr(app_name.size()-7, 7);
      if (app_sub == std::string("lb_iter")) {
        lb_ = default_lb;
      }
    }
    if  (app_name.size() >= 12) {
      auto const app_sub = app_name.substr(app_name.size()-12, 12);
      if (app_sub == std::string("test_lb_lite")) {
        lb_ = default_lb;
      }
    }
    if (app_name.size() > 2) {
      auto const last = app_name.size() - 1;
      if (app_name[last-1] == '_') {
        switch (app_name[last]) {
        case 'h': lb_ = LBType::HierarchicalLB; break;
        case 'g': lb_ = LBType::GreedyLB;       break;
        case 'r': lb_ = LBType::RotateLB;       break;
        default:                                break;
        }
      }
    }
  }

  if (not is_interop) {
    MPI_Init(&argc, &argv);
  }

  if (comm != nullptr) {
    communicator_ = *comm;
  } else {
    communicator_ = MPI_COMM_WORLD;
  }

  MPI_Barrier(communicator_);

  if (is_interop) {
    MPI_Comm_split(communicator_, 0, 0, &communicator_);
  }

  MPI_Barrier(communicator_);

  is_comm_world_ = communicator_ == MPI_COMM_WORLD;

  int numNodesLocal = uninitialized_destination;
  int thisNodeLocal = uninitialized_destination;

  MPI_Comm_size(communicator_, &numNodesLocal);
  MPI_Comm_rank(communicator_, &thisNodeLocal);

  numNodes_ = static_cast<NodeType>(numNodesLocal);
  thisNode_ = static_cast<NodeType>(thisNodeLocal);

  setDefaultWorker();
}

Context::Context(bool const interop, MPI_Comm* comm)
  : Context(0, nullptr, interop, comm)
{ }

void Context::setDefaultWorker() {
  setWorker(worker_id_comm_thread);
}

DeclareClassOutsideInitTLS(Context, WorkerIDType, thisWorker_, no_worker_id);

}}  // end namespace vt::ctx

#undef DEBUG_VT_CONTEXT
