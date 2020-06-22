\page pool Memory Pool
\brief Memory pool for efficient allocation

The memory pool component `vt::pool::Pool`, accessed via `vt::thePool()`
provides a highly efficient memory pool for fixed sized allocations in three
sizes: small (`vt::pool::memory_size_small`), medium
(`vt::pool::memory_size_medium`), and large (currently unimplemented).

All message allocation (on the send and receive side) is overloaded with
new/delete overloads to allocate message memory through the \vt memory pool. The
pool implementation uses a non-thread-safe allocation policy (must be
allocated/deallocated on the same thread) with fixed sized buckets. If the size
exceeds the largest bucket, the memory pool will fall back on the standard
allocator.
