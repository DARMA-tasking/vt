
#if !defined INCLUDED_CONTEXT
#define INCLUDED_CONTEXT

#include <memory>
#include <mpi.h>

#include "config.h"

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

 private:
  NodeType thisNode_ = uninitialized_destination;
  NodeType numNodes_ = uninitialized_destination;
  WorkerCountType numWorkers_ = no_workers;

  bool is_comm_world_ = true;
  MPI_Comm communicator_ = MPI_COMM_WORLD;
};

}} // end namespace vt::ctx

namespace vt {

extern std::unique_ptr<ctx::Context> theContext;

} // end namespace vt

#endif /*INCLUDED_CONTEXT*/

