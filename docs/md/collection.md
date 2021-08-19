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

\subsection collection-configuration Configuring Collections

The preferred interface for constructing a new collection is
`vt::makeCollection<T>()`. Using a fluent-style interface, `makeCollection`
returns a configuration object to set properties of the collection before
actually constructing the collection. Once it is configured, the `.wait()` or
`.deferWithEpoch(callback)` methods allows one to either block until the
collection is created or wait on the returned epoch until construction finishes
(respectively). The `wait()` variant returns the proxy for use immediately
(after blocking), whereas `deferWithEpoch(callback)` supplies the proxy when the
epoch terminates by triggering the callback passed to it.

\subsubsection collection-collective-vs-rooted Collective vs. Rooted

The function `vt::makeCollection<T>()` will create a collection in a collective
fashion, meaning it must be called in tandem on all nodes. Alternatively, one
may call `vt::makeCollectionRooted<T>()` to construct a rooted collection, which
is invoked only on a single rank. (and the proxy is returned to a single
rank). After waiting for construction, elements will have been constructed on
their appropriate ranks, and the provided collection proxy will be usable on any
rank it's sent to.

\subsubsection collection-bounds-insertion Bounds and Insertion

For collections without dynamic membership at runtime, one must call
`.bounds(my_range)` to specify the bounds in each dimension for the collection
or specify exactly one bulk insertion range (`.bulkInsert(my_range_1)`), where
`my_range_1` will be the assumed bounds for the collection. Bulk insertion is
one such way to specify how insertions should happen during construction. The
`.bulkInsert()` method (with no parameter) tells the runtime to insert all
collection elements within the bounds using the mapping function to determine
placement. The user can also specify specific ranges to bulk insert using
`.bulkInsert(my_range_1)` with a parameter (this can be called multiple
times).

For collective collection constructions, one may also use list insertion
(`.listInsert(my_index_list)`) to specify non-contiguous lists of indices that
the runtime should insert at construction time. Finally, for collective
constructions, one may call `.listInsertHere(my_index_list)` to specifically
instruct the runtime to construct a list of collection elements on the
particular rank where it is invoked. This overrides the mapping function that the
user supplies.

\subsubsection collection-mapping Mapping Functions and Object Groups

By default, a mapping function is applied to every collection. If the collection
has bounds, the system will choose a default blocked mapping (across all
dimensions) for initial placement. For collections without bounds (ones with
dynamic membership), the system uses a simple xor hash function to generate a
valid initial location for each index deterministically. One may specify a
mapping function in two ways: the user can provide a stateless function as a
template argument to `.mapperFunc<my_map>()`, where `my_map` has the following
definition (shown for a 1-dimensional collection):

\code{.cpp}
vt::NodeType my_map(vt::Index1D* idx, vt::Index1D* bounds, vt::NodeType num_nodes) {
    return idx->x() % num_nodes;
}
\endcode

Or, alternatively, one may specify a object group mapper, which contains an
instance across all ranks that may communicate to determine placement. The
`.mapperObjGroup(proxy)` method configures the mapping object with an object
group instance that already exists. Otherwise, one may just give the type and
constructor arguments to create a new instance:
`.mapperObjGroup<MyObjectGroup>(args...)`. An object group mapper must inherit
from `vt::mapping::BaseMapper` and implement the pure virtual method `NodeType
map(IdxT* idx, int ndim, NodeType num_nodes)` to define the mapping for the
runtime. As an example, the object group mapper used by default for unbounded
collections is implemented as follows:

\code{.cpp}
template <typename IdxT>
struct UnboundedDefaultMap : vt::mapping::BaseMapper<IdxT> {
  static ObjGroupProxyType construct() {
    auto proxy = theObjGroup()->makeCollective<UnboundedDefaultMap<IdxT>>();
    return proxy.getProxy();
  }

  NodeType map(IdxT* idx, int ndim, NodeType num_nodes) override {
    typename IdxT::DenseIndexType val = 0;
    for (int i = 0; i < ndim; i++) {
      val ^= idx->get(i);
    }
    return val % num_nodes;
  }
};
\endcode

Note that all collection mapping functions or object groups must be
deterministic across all nodes for the same inputs.

\subsubsection collection-element-construction Element Construction

By default, the collection type `T` (that inherits from the runtime base type
`vt::Collection<T, IndexType>`) must have a default constructor. However, this
can be avoided by configuring the collection with a specialized element
constructor using `.elementConstructor(x)`, where `x`'s type is
`std::function<std::unique_ptr<ColT>(IndexT idx)>` and `ColT` is the collection
type and `IndexT` is the index type for the collection. This configuration is
only valid for collective constructions because the element constructor function
can not be safely sent over the network. If this is provided, the collection
manager will not try to default construct the collection elements, instead
calling the user-provided constructor passed to this function.

\subsubsection collection-element-migratability Element Migratability

By default, all collection elements are migratable and can be moved by the load
balancer when it is invoked by the user. However, one may inform VT that
collection is entirely non-migratable by setting the parameter
`.migratable(false)` during construction. By doing this, work executed by its
elements will be recorded as background load on the initially mapped rank and
excluded from the load balancer migration decisions.

\subsubsection collection-dynamic-membership Dynamic Membership

By default, collections do not have dynamic membership: they might be dense or
sparse within the specified bounds, but the set of collection elements that are
created at construction time persists (and never grows or shrinks) until the
collection is completely destroyed. Dynamic membership allows the user to
specify insertions and deletions as the program executes in a safe and orderly
manner. To enable this, one must call `.dynamicMembership(true)`. Note that the
previous requirement of specifying collection bounds becomes optional with
dynamic membership.

Once a collection is constructed with dynamic membership, one must start a
collective modification epoch to make changes to the collection's
membership. This is performed in the following way (note that this is a
collective interface):

\code{.cpp}
  auto proxy = vt::makeCollection<MyCollection>()
    .dynamicMembership(true)
    .collective(true)
    .wait();

  auto range = vt::Index1D(num_elms);
  auto token = proxy.beginModification();
  for (int i = 0; i < range.x() / 2; i++) {
    if (i % num_nodes == this_node) {
        proxy[i].insertAt(token, i % 2);
    }
  }
  proxy.finishModification(std::move(token));
\endcode

The calls to `proxy.beginModification()` start the insertion/deletion epoch by
returning a token that must be passed to the actual modification calls. To
insert a new collection element, the interface provides several methods on the
indexed proxy: `insert`, `insertAt`, `insertMsg` or `insertAtMsg`. The `insert`
method performs the insertion at the location specified by the mapping
function/mapping object group that is provided when the collection is
constructed. The `insertAt` or `insertAtMsg` allow the user to specify exactly
where the new element should reside overriding the default mapping for the
element. The `insertMsg` or `insertAtMsg` calls allow the user to pass a message
to the collection element which invokes a non-default constructor that has the
user-specified message as an argument.

Finally, one may call `destroy` on the indexed proxy to delete an element. All
these modifications take place after `finishModification` is invoked---a blocking
call that enacts the changes across the system. Once `finishModification`
returns, the runtime guarantees that all changes have taken place across the
system and all spanning trees are reconstructed or modified based on the changes
made.

If a reduction is ongoing while insertions or deletions happen, the new elements
are still expected to contribute. That is, new collection elements are part of
the next sequenced reduction that has not causally terminated across the
distributed system. For code readability, we generally recommend that the user
wait on termination of any reductions before membership modifications are made.

\section rooted-hello-world-collection Hello World 1D Dense Collection (Rooted)
\snippet  examples/hello_world/hello_world_collection.cc Hello world collection

\section collective-hello-world-collection Hello World 1D Dense Collection (Collective)
\snippet  examples/hello_world/hello_world_collection_collective.cc Hello world collective collection

\section reduce-hello-world-collection Hello World 1D Collection Reduce
\snippet  examples/hello_world/hello_world_collection_reduce.cc Hello world reduce collection

\section staged-insert-hello-world-collection Hello World 1D Collection Staged Insert
\snippet  examples/hello_world/hello_world_collection_staged_insert.cc Hello world staged insert collection
