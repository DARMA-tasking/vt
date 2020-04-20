#include "vt/config.h"
#include "vt/runtime/mpi_access.h"

#include <mpi.h>

#define EXTERN extern "C"
#define AUTOGEN

#if backend_check_enabled(mpi_access_guards)

AUTOGEN EXTERN int MPI_Abort(MPI_Comm comm, int errorcode) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Abort' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Abort(comm, errorcode);
}
AUTOGEN EXTERN int MPI_Address(void *location, MPI_Aint *address) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Address' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Address(location, address);
}
AUTOGEN EXTERN int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Allgather' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}
AUTOGEN EXTERN int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Allgatherv' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
}
AUTOGEN EXTERN int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Allreduce' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
}
AUTOGEN EXTERN int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Alltoall' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}
AUTOGEN EXTERN int MPI_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Alltoallv' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
}
AUTOGEN EXTERN int MPI_Alltoallw(const void *sendbuf, const int *sendcounts, const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Alltoallw' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
}
AUTOGEN EXTERN int MPI_Barrier(MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Barrier' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Barrier(comm);
}
AUTOGEN EXTERN int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Bcast' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Bcast(buffer, count, datatype, root, comm);
}
AUTOGEN EXTERN int MPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Bsend' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Bsend(buf, count, datatype, dest, tag, comm);
}
AUTOGEN EXTERN int MPI_Bsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Bsend_init' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Bsend_init(buf, count, datatype, dest, tag, comm, request);
}
AUTOGEN EXTERN int MPI_Buffer_attach(void *buffer, int size) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Buffer_attach' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Buffer_attach(buffer, size);
}
AUTOGEN EXTERN int MPI_Buffer_detach(void *buffer, int *size) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Buffer_detach' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Buffer_detach(buffer, size);
}
AUTOGEN EXTERN int MPI_Cancel(MPI_Request *request) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Cancel' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Cancel(request);
}
AUTOGEN EXTERN int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int *coords) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Cart_coords' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Cart_coords(comm, rank, maxdims, coords);
}
AUTOGEN EXTERN int MPI_Cart_create(MPI_Comm old_comm, int ndims, const int *dims, const int *periods, int reorder, MPI_Comm *comm_cart) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Cart_create' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Cart_create(old_comm, ndims, dims, periods, reorder, comm_cart);
}
AUTOGEN EXTERN int MPI_Cart_get(MPI_Comm comm, int maxdims, int *dims, int *periods, int *coords) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Cart_get' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Cart_get(comm, maxdims, dims, periods, coords);
}
AUTOGEN EXTERN int MPI_Cart_map(MPI_Comm comm, int ndims, const int *dims, const int *periods, int *newrank) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Cart_map' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Cart_map(comm, ndims, dims, periods, newrank);
}
AUTOGEN EXTERN int MPI_Cart_rank(MPI_Comm comm, const int *coords, int *rank) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Cart_rank' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Cart_rank(comm, coords, rank);
}
AUTOGEN EXTERN int MPI_Cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Cart_shift' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Cart_shift(comm, direction, disp, rank_source, rank_dest);
}
AUTOGEN EXTERN int MPI_Cart_sub(MPI_Comm comm, const int *remain_dims, MPI_Comm *new_comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Cart_sub' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Cart_sub(comm, remain_dims, new_comm);
}
AUTOGEN EXTERN int MPI_Cartdim_get(MPI_Comm comm, int *ndims) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Cartdim_get' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Cartdim_get(comm, ndims);
}
AUTOGEN EXTERN int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Comm_compare' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Comm_compare(comm1, comm2, result);
}
AUTOGEN EXTERN int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Comm_create' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Comm_create(comm, group, newcomm);
}
AUTOGEN EXTERN int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Comm_dup' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Comm_dup(comm, newcomm);
}
AUTOGEN EXTERN int MPI_Comm_free(MPI_Comm *comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Comm_free' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Comm_free(comm);
}
AUTOGEN EXTERN int MPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Comm_get_name' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Comm_get_name(comm, comm_name, resultlen);
}
AUTOGEN EXTERN int MPI_Comm_group(MPI_Comm comm, MPI_Group *group) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Comm_group' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Comm_group(comm, group);
}
AUTOGEN EXTERN int MPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Comm_remote_group' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Comm_remote_group(comm, group);
}
AUTOGEN EXTERN int MPI_Comm_remote_size(MPI_Comm comm, int *size) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Comm_remote_size' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Comm_remote_size(comm, size);
}
AUTOGEN EXTERN int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Comm_split' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Comm_split(comm, color, key, newcomm);
}
AUTOGEN EXTERN int MPI_Comm_test_inter(MPI_Comm comm, int *flag) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Comm_test_inter' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Comm_test_inter(comm, flag);
}
AUTOGEN EXTERN int MPI_Dims_create(int nnodes, int ndims, int *dims) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Dims_create' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Dims_create(nnodes, ndims, dims);
}
AUTOGEN EXTERN int MPI_Errhandler_create(MPI_Handler_function *function, MPI_Errhandler *errhandler) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Errhandler_create' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Errhandler_create(function, errhandler);
}
AUTOGEN EXTERN int MPI_Errhandler_free(MPI_Errhandler *errhandler) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Errhandler_free' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Errhandler_free(errhandler);
}
AUTOGEN EXTERN int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Errhandler_get' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Errhandler_get(comm, errhandler);
}
AUTOGEN EXTERN int MPI_Error_class(int errorcode, int *errorclass) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Error_class' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Error_class(errorcode, errorclass);
}
AUTOGEN EXTERN int MPI_Error_string(int errorcode, char *string, int *resultlen) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Error_string' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Error_string(errorcode, string, resultlen);
}
AUTOGEN EXTERN int MPI_Finalize() {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Finalize' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Finalize();
}
AUTOGEN EXTERN int MPI_Finalized(int *flag) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Finalized' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Finalized(flag);
}
AUTOGEN EXTERN int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Gather' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}
AUTOGEN EXTERN int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Gatherv' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);
}
AUTOGEN EXTERN int MPI_Get_processor_name(char *name, int *resultlen) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Get_processor_name' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Get_processor_name(name, resultlen);
}
AUTOGEN EXTERN int MPI_Get_version(int *version, int *subversion) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Get_version' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Get_version(version, subversion);
}
AUTOGEN EXTERN int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int *index, int *edges) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Graph_get' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Graph_get(comm, maxindex, maxedges, index, edges);
}
AUTOGEN EXTERN int MPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Graph_neighbors_count' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Graph_neighbors_count(comm, rank, nneighbors);
}
AUTOGEN EXTERN int MPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, int *neighbors) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Graph_neighbors' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Graph_neighbors(comm, rank, maxneighbors, neighbors);
}
AUTOGEN EXTERN int MPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Graphdims_get' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Graphdims_get(comm, nnodes, nedges);
}
AUTOGEN EXTERN int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Group_compare' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Group_compare(group1, group2, result);
}
AUTOGEN EXTERN int MPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Group_difference' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Group_difference(group1, group2, newgroup);
}
AUTOGEN EXTERN int MPI_Group_free(MPI_Group *group) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Group_free' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Group_free(group);
}
AUTOGEN EXTERN int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Group_intersection' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Group_intersection(group1, group2, newgroup);
}
AUTOGEN EXTERN int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Group_range_excl' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Group_range_excl(group, n, ranges, newgroup);
}
AUTOGEN EXTERN int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Group_range_incl' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Group_range_incl(group, n, ranges, newgroup);
}
AUTOGEN EXTERN int MPI_Group_rank(MPI_Group group, int *rank) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Group_rank' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Group_rank(group, rank);
}
AUTOGEN EXTERN int MPI_Group_size(MPI_Group group, int *size) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Group_size' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Group_size(group, size);
}
AUTOGEN EXTERN int MPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Group_union' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Group_union(group1, group2, newgroup);
}
AUTOGEN EXTERN int MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Ibsend' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Ibsend(buf, count, datatype, dest, tag, comm, request);
}
AUTOGEN EXTERN int MPI_Info_create(MPI_Info *info) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Info_create' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Info_create(info);
}
AUTOGEN EXTERN int MPI_Info_dup(MPI_Info info, MPI_Info *newinfo) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Info_dup' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Info_dup(info, newinfo);
}
AUTOGEN EXTERN int MPI_Info_free(MPI_Info *info) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Info_free' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Info_free(info);
}
AUTOGEN EXTERN int MPI_Info_get_nkeys(MPI_Info info, int *nkeys) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Info_get_nkeys' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Info_get_nkeys(info, nkeys);
}
AUTOGEN EXTERN int MPI_Info_get_nthkey(MPI_Info info, int n, char *key) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Info_get_nthkey' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Info_get_nthkey(info, n, key);
}
AUTOGEN EXTERN int MPI_Init(int *argc, char ***argv) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Init' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Init(argc, argv);
}
AUTOGEN EXTERN int MPI_Initialized(int *flag) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Initialized' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Initialized(flag);
}
AUTOGEN EXTERN int MPI_Init_thread(int *argc, char ***argv, int required, int *provided) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Init_thread' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Init_thread(argc, argv, required, provided);
}
AUTOGEN EXTERN int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm bridge_comm, int remote_leader,  int tag, MPI_Comm *newintercomm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Intercomm_create' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Intercomm_create(local_comm, local_leader, bridge_comm, remote_leader, tag, newintercomm);
}
AUTOGEN EXTERN int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintercomm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Intercomm_merge' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Intercomm_merge(intercomm, high, newintercomm);
}
AUTOGEN EXTERN int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Iprobe' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Iprobe(source, tag, comm, flag, status);
}
AUTOGEN EXTERN int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Irecv' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
}
AUTOGEN EXTERN int MPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Irsend' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Irsend(buf, count, datatype, dest, tag, comm, request);
}
AUTOGEN EXTERN int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Isend' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
}
AUTOGEN EXTERN int MPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Issend' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Issend(buf, count, datatype, dest, tag, comm, request);
}
AUTOGEN EXTERN int MPI_Keyval_create(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn, int *keyval, void *extra_state) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Keyval_create' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Keyval_create(copy_fn, delete_fn, keyval, extra_state);
}
AUTOGEN EXTERN int MPI_Keyval_free(int *keyval) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Keyval_free' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Keyval_free(keyval);
}
AUTOGEN EXTERN int MPI_Op_create(MPI_User_function *function, int commute, MPI_Op *op) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Op_create' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Op_create(function, commute, op);
}
AUTOGEN EXTERN int MPI_Op_free(MPI_Op *op) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Op_free' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Op_free(op);
}
AUTOGEN EXTERN int MPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Pack_size' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Pack_size(incount, datatype, comm, size);
}
AUTOGEN EXTERN int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Probe' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Probe(source, tag, comm, status);
}
AUTOGEN EXTERN int MPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Recv_init' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Recv_init(buf, count, datatype, source, tag, comm, request);
}
AUTOGEN EXTERN int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Recv' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Recv(buf, count, datatype, source, tag, comm, status);
}
AUTOGEN EXTERN int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Reduce' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
}
AUTOGEN EXTERN int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Reduce_scatter' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm);
}
AUTOGEN EXTERN int MPI_Request_free(MPI_Request *request) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Request_free' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Request_free(request);
}
AUTOGEN EXTERN int MPI_Rsend(const void *ibuf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Rsend' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Rsend(ibuf, count, datatype, dest, tag, comm);
}
AUTOGEN EXTERN int MPI_Rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Rsend_init' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Rsend_init(buf, count, datatype, dest, tag, comm, request);
}
AUTOGEN EXTERN int MPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Scan' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
}
AUTOGEN EXTERN int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Scatter' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}
AUTOGEN EXTERN int MPI_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Scatterv' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
}
AUTOGEN EXTERN int MPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Send_init' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Send_init(buf, count, datatype, dest, tag, comm, request);
}
AUTOGEN EXTERN int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Send' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Send(buf, count, datatype, dest, tag, comm);
}
AUTOGEN EXTERN int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Sendrecv' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status);
}
AUTOGEN EXTERN int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Sendrecv_replace' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);
}
AUTOGEN EXTERN int MPI_Ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Ssend_init' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Ssend_init(buf, count, datatype, dest, tag, comm, request);
}
AUTOGEN EXTERN int MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Ssend' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Ssend(buf, count, datatype, dest, tag, comm);
}
AUTOGEN EXTERN int MPI_Start(MPI_Request *request) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Start' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Start(request);
}
AUTOGEN EXTERN int MPI_Startall(int count, MPI_Request *array_of_requests) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Startall' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Startall(count, array_of_requests);
}
AUTOGEN EXTERN int MPI_Status_set_cancelled(MPI_Status *status, int flag) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Status_set_cancelled' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Status_set_cancelled(status, flag);
}
AUTOGEN EXTERN int MPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, int count) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Status_set_elements' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Status_set_elements(status, datatype, count);
}
AUTOGEN EXTERN int MPI_Testall(int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[]) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Testall' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Testall(count, array_of_requests, flag, array_of_statuses);
}
AUTOGEN EXTERN int MPI_Testany(int count, MPI_Request array_of_requests[], int *index, int *flag, MPI_Status *status) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Testany' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Testany(count, array_of_requests, index, flag, status);
}
AUTOGEN EXTERN int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Test' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Test(request, flag, status);
}
AUTOGEN EXTERN int MPI_Test_cancelled(const MPI_Status *status, int *flag) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Test_cancelled' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Test_cancelled(status, flag);
}
AUTOGEN EXTERN int MPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Testsome' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
}
AUTOGEN EXTERN int MPI_Topo_test(MPI_Comm comm, int *status) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Topo_test' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Topo_test(comm, status);
}
AUTOGEN EXTERN int MPI_Waitall(int count, MPI_Request *array_of_requests, MPI_Status *array_of_statuses) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Waitall' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Waitall(count, array_of_requests, array_of_statuses);
}
AUTOGEN EXTERN int MPI_Waitany(int count, MPI_Request *array_of_requests, int *index, MPI_Status *status) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Waitany' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Waitany(count, array_of_requests, index, status);
}
AUTOGEN EXTERN int MPI_Wait(MPI_Request *request, MPI_Status *status) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Wait' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Wait(request, status);
}
AUTOGEN EXTERN int MPI_Waitsome(int incount, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses) {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Waitsome' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
}
AUTOGEN EXTERN double MPI_Wtick() {
  vtAssert(
    vt::runtime::ScopedMPIAccess::mpiCallsAllowed(),
    "The MPI function 'MPI_Wtick' was called from a VT handler."
    " MPI functions should not used inside user code invoked from VT handlers."
  );

  return PMPI_Wtick();
}
#endif // backend_check_enabled(mpi_access_guards)

