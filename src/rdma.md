\page rdma RDMA Manager
\brief Node-level RDMA

@m_class{m-label m-danger} **Experimental**

The RDMA manager component `vt::rdma::RDMAManager`, accessed via `vt::theRDMA()`
is an experimental component that sends pure data to registered RDMA handlers or
directly to memory locations.

Registered RDMA handlers trigger a function when the data arrives (GET) or is
sent (PUT). If registered memory locations are used directly, one may create a
RDMA channel which backs the GET/PUT by `MPI_Get`/`MPI_Put`.

