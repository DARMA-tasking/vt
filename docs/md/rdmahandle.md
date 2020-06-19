\page rdmahandle RDMA Handle Manager
\brief RDMA handles backed by MPI

The RDMA handle manager component `vt::rdma::Manager`, accessed via
`vt::theHandleRDMA()` is a component that allows data to be transferred between
RDMA handles, which are persistent objects with underlying memory registered
with MPI.

RDMA handles can either be node- or index-level, depending on whether they
belong to an objgroup or collection. A handle provides an interface to calling
get/put/accum to access the backing MPI implementation.
