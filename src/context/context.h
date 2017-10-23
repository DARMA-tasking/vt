
#if !defined INCLUDED_CONTEXT
#define INCLUDED_CONTEXT

#include <memory>
#include <mpi.h>

#include "config.h"
#include "context/context_attorney_fwd.h"
#include "utils/tls/tls.h"

namespace vt {  namespace ctx {

struct Context {
  Context(int argc, char** argv, bool const interop, MPI_Comm* comm = nullptr);
  Context(bool const interop, MPI_Comm* comm = nullptr);

  inline NodeType getNode() const { return thisNode_; }
  inline NodeType getNumNodes() const { return numNodes_; }

  inline MPI_Comm getComm() const { return communicator_; }
  inline bool isCommWorld() const { return is_comm_world_; }

  inline void setNumWorkers(WorkerCountType w) { numWorkers_ = w; }
  inline WorkerCountType getNumWorkers() const { return numWorkers_; }
  inline bool hasWorkers() const { return numWorkers_ != no_workers; }
  inline WorkerIDType getWorker() const {
    return AccessClassTLS(Context, thisWorker_);
  }

  friend struct ContextAttorney;

protected:
  void setWorker(WorkerIDType const worker) {
    AccessClassTLS(Context, thisWorker_) = worker;
  }

private:
  NodeType thisNode_ = uninitialized_destination;
  NodeType numNodes_ = uninitialized_destination;
  WorkerCountType numWorkers_ = no_workers;
  bool is_comm_world_ = true;
  MPI_Comm communicator_ = MPI_COMM_WORLD;
  DeclareClassInsideInitTLS(Context, WorkerIDType, thisWorker_, no_worker_id);
};

}} // end namespace vt::ctx

namespace vt {

extern ctx::Context* theContext();

} // end namespace vt

#endif /*INCLUDED_CONTEXT*/

