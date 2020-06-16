\page group GroupManager
\brief Create a grouping of nodes

The group manager component `vt::group::GroupManager`, accessed via
`vt::theGroup()` manages both rooted and collective groups (or subsets) of nodes
that can be broadcast to or reduced over. The group manager implements a fully
distributed algorithm for constructing groups collectively with each node
deciding if it should be included. The group manager builds a reasonably
balanced distributed spanning tree based on these collective votes.

One major use case for the group manager is creating spanning trees for
reductions over virtual collections that do not span all the nodes.

When creating a group, one may ask \vt to create a underlying MPI group, which
can be accessed once the group has finished construction.

\section collective-group-example Example creating a collective group

\snippet examples/group/group_collective.cc Collective group creation
