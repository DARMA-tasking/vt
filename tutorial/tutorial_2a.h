
#include "vt/transport.h"

namespace vt { namespace tutorial {

// Forward declaration for message
struct MyCollMsg;

//               VT Base Class for a collection
//         \-------------------------------------/
//          \                                   /
//           \                         Index   /
//            \                     \---------/
//             \                     \       /
struct MyCol : ::vt::Collection<MyCol,Index1D> {

  void msgHandler(MyCollMsg* msg);

};

//                 VT Base Message for Collections
//               \--------------------------------/
//                \                              /
struct MyCollMsg : ::vt::CollectionMessage<MyCol> { };

void MyCol::msgHandler(MyCollMsg* msg) {
  auto cur_node = theContext()->getNode();
  auto idx = this->getIndex();
  ::fmt::print("MyCol::msgHandler index={}, node={}\n", idx.x(), cur_node);
}

// Tutorial code to demonstrate creating a collection
static inline void collection() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();

  /*
   * This is an example of creating a virtual context collection with an index
   * range
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
