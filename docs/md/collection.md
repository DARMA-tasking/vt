\page collection Virtual Context Collection
\brief Collection of tasks

The virtual context collection component
`vt::vrt::collection::CollectionManager`, accessed via `vt::theCollection()` is
a core VT component that manages multi-dimensional collections of *virtual
context* (or a migratable C++ object registered with \vt) elements. It manages
the creation, deletion, and messaging across elements at runtime supporting
dense, sparse, on-demand, and staged insert modes. It utilizes the \ref
location to manage the location of these elements to efficiently deliver
messages. It also utilizes the \ref group to build a spanning tree across
the nodes that the collection is currently mapped to. This group makes
broadcasts efficient and allows reductions to make progress without waiting for
nodes that do not have collection elements.

The \ref proc-stats component stores the statistics for live collections that
then passes the instrumented data to the \ref lb-manager component to apply load
balancing strategies. You can use `--vt_lb_keep_last_elm` flag to prohibit load
balancer from migrating last element in collection.

\section rooted-hello-world-collection Hello World 1D Dense Collection (Rooted)
\snippet  examples/hello_world/hello_world_collection.cc Hello world collection

\section collective-hello-world-collection Hello World 1D Dense Collection (Collective)
\snippet  examples/hello_world/hello_world_collection_collective.cc Hello world collective collection

\section reduce-hello-world-collection Hello World 1D Collection Reduce
\snippet  examples/hello_world/hello_world_collection_reduce.cc Hello world reduce collection

\section staged-insert-hello-world-collection Hello World 1D Collection Staged Insert
\snippet  examples/hello_world/hello_world_collection_staged_insert.cc Hello world staged insert collection
