
#include "vt/transport.h"

namespace vt { namespace tutorial {

// Forward declaration for message
struct MyCollMsg;

//               VT Base Class for a collection     Index (built-in)
//            \----------------------------------/   \---------/
//             \                                /     \       /
struct MyCol : ::vt::Collection<MyCol,Index1D> {

  void msgHandler(MyCollMsg* msg);

};

//                            VT Base Message for Collections
//               \-------------------------------------------/
//                \                                         /
struct MyCollMsg : ::vt::CollectionMessage<MyCol> { };

void MyCol::msgHandler(MyCollMsg* msg) {
  auto cur_node = theContext()->getNode();
  auto idx = this->getIndex();
  ::fmt::print("MyCol::msgHandler index={}, node={}\n", idx.x(), cur_node);
}

// Tutorial code to demonstrate broadcasting a message to the entire system
static inline void collection() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();

  /*
   * This is an example of the rooted group creation and broadcast to that
   * group. A group allows the user to create a subset of nodes. A broadcast by
   * default sends the message to every node in the default group (which
   * includes all nodes). If a explicit group is set in the envelope, the
   * broadcast will only arrive on the nodes in that group.
   */

  if (this_node == 0) {
    // Range of 32 elements for the collection
    auto range = ::vt::Index1D(32);
    // Construct the collection: invoked by one node. By default, the elements
    // will be block mapped to the nodes
    auto proxy = theCollection()->construct<MyCol>(range);

    // Broadcast a message to the entire collection. The msgHandler will be
    // invoked on every element to the collection
    auto msg = ::vt::makeSharedMessage<MyCollMsg>();
    proxy.broadcast<MyCollMsg,&MyCol::msgHandler>(msg);

    // Send a message to the 5th element of the collection
    auto msg2 = ::vt::makeSharedMessage<MyCollMsg>();
    proxy[5].send<MyCollMsg,&MyCol::msgHandler>(msg2);
  }
}

}} /* end namespace vt::tutorial */
