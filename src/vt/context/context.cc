
#include "vt/context/context.h"

#include <string>
#include <cstring>

#include <mpi.h>
#include <stdio.h>

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
    int name_len = 20;
    char name[20];
    gethostname(name, name_len);
    std::cout << "MPI_Init =" <<name<< std::endl;

  std::cout << " context cc MPI" << std::endl;
  if (not is_interop) {
    MPI_Init(&argc, &argv);
  }
  std::cout << " context cc MPI 2" << std::endl;

  if (comm != nullptr) {
    communicator_ = *comm;
  } else {
    communicator_ = MPI_COMM_WORLD;
  }
  std::cout << " context cc MPI Communicator" << std::endl;

  MPI_Barrier(communicator_);

  if (is_interop) {
    MPI_Comm_split(communicator_, 0, 0, &communicator_);
  }
  std::cout << " context cc MPI split" << std::endl;

  MPI_Barrier(communicator_);

  is_comm_world_ = communicator_ == MPI_COMM_WORLD;

  int numNodesLocal = uninitialized_destination;
  int thisNodeLocal = uninitialized_destination;

  MPI_Comm_size(communicator_, &numNodesLocal);
  std::cout << " context cc  MPI_Comm_size " << std::endl;
  MPI_Comm_rank(communicator_, &thisNodeLocal);
  std::cout << " context cc  MPI_Comm_rank" << std::endl;

  std::cout << "myd id is  before sleep" << getpid()<< std::endl;
   int i = 0;
   while(0==i)
   {
    sleep(10);
   }

   int myId;
   MPI_Comm_rank(MPI_COMM_WORLD,&myId);
   if(myId == 0)
   {
     std::cout << "myd id is 0"<< std::endl;
     int *a=nullptr;
         a[10] = 20;
   }
   else
   {
     std::cout << "myd id is N"<< std::endl;
   }


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

namespace vt { namespace debug {

NodeType preNode() {
  return ::vt::curRT != nullptr ? theContext()->getNode() : -1;
}
NodeType preNodes() {
  return ::vt::curRT != nullptr ? theContext()->getNumNodes() : -1;
}

}} /* end namespace vt::debug */


#undef DEBUG_VT_CONTEXT
