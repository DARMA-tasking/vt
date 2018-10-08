
#include "transport.h"

namespace vt { namespace tutorial {

//                  VT Base Message
//                 \----------------/
//                  \              /
struct MyDataMsg : ::vt::Message {
  MyDataMsg() = default;
  MyDataMsg(double in_x, double in_y, double in_z)
    : x(in_x), y(in_y), z(in_z)
  { }

  double getX() const { return x; }
  double getY() const { return y; }
  double getZ() const { return z; }

private:
  double x = 0.0, y = 0.0, z = 0.0;
};

// Forward declaration for the active message handler
static void msgHandlerX(MyDataMsg* msg);

// Tutorial code to demonstrate broadcasting a message to the entire system
static inline void activeMessageBroadcast() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();

  /*
   * The theMsg()->broadcastMsg(..) will send the message to every node in the
   * system. Every node will include all the nodes that VT has depending on the
   * MPI communicator passed in or the size attained (number of ranks) when
   * executing MPI init directly in non-interoperability mode.
   */

  if (this_node == 0) {
    auto msg = ::vt::makeSharedMessage<MyDataMsg>(1.0,2.0,3.0);
    ::vt::theMsg()->broadcastMsg<MyDataMsg,msgHandlerX>(msg);
  }
}

// Message handler
static void msgHandlerX(MyDataMsg* msg) {
  vtAssert(
    msg->getX() == 1.0 && msg->getY() == 2.0 && msg->getZ() == 3.0,
    "Values x,y,z incorrect"
  );

  auto const cur_node = ::vt::theContext()->getNode();
  ::fmt::print("msgHandlerX: triggered on node={}\n", cur_node);
}

}} /* end namespace vt::tutorial */
