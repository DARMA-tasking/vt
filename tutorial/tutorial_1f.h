
#include "transport.h"

namespace vt { namespace tutorial {

//                  VT Base Message
//                 \----------------/
//                  \              /
struct MySimpleMsg2 : ::vt::Message { };

// Forward declaration for the active message handler
static void msgHandlerGroupB(MySimpleMsg2* msg);

// Tutorial code to demonstrate broadcasting a message to the entire system
static inline void activeMessageGroupCollective() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();

  /*
   * This is an example of the collective group creation and broadcast to that
   * group. A group allows the user to create a subset of nodes. The collective
   * group allows all nodes to participate in creating the group by passing a
   * boolean that indicates if they are apart of the group.
   *
   * Unlike the rooted group creation (which requires an initial set at a root
   * node), the collective group is fully distributed: its creation and
   * storage. The set of all nodes included is never stored in a central
   * location. This is managed by efficient distributed algorithms that create a
   * spanning tree based on the filter and rebalance it depending on outcomes.
   */

  auto const& is_even_node = this_node % 2 == 0;

  auto group = theGroup()->newGroupCollective(
    is_even_node, [](GroupType group_id){
      fmt::print("Group is created: id={:x}\n", group_id);

      // In this example, node 1 broadcasts to the group of even nodes
      auto const this_node = ::vt::theContext()->getNode();
      if (this_node == 1) {
        auto msg = makeSharedMessage<MySimpleMsg2>();
        envelopeSetGroup(msg->env, group_id);
        theMsg()->broadcastMsg<MySimpleMsg2,msgHandlerGroupB>(msg);
      }
    }
  );
}

// Message handler
static void msgHandlerGroupB(MySimpleMsg2* msg) {
  auto const cur_node = ::vt::theContext()->getNode();
  vtAssert(cur_node % 2 == 0, "This handler should only execute on even nodes");

  ::fmt::print("msgHandlerGroupB: triggered on node={}\n", cur_node);
}

}} /* end namespace vt::tutorial */
