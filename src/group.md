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

\code{.cpp}
int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();

  bool odd_node_filter = this_node % 2 == 1;

  vt::GroupType new_group = vt::theGroup()->newGroupCollective(
    odd_node_filter, [=](vt::GroupType group){
      auto const& root = 0;
      auto const& in_group = vt::theGroup()->inGroup(group);
      auto const& root_node = vt::theGroup()->groupRoot(group);
      auto const& is_default_group = vt::theGroup()->groupDefault(group);
      fmt::print(
        "{}: Group is created: group={:x}, in_group={}, root={}, "
        "is_default_group={}\n",
        this_node, group, in_group, root_node, is_default_group
      );
    }
  );

  fmt::print("{}: Created new collective group={:x}\n", this_node, new_group);

  vt::finalize();

  return 0;
}
\endcode
