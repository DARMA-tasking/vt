
#include "transport.h"

namespace vt { namespace tutorial {

/*
 * This is a user-defined message that can be sent to a node via VT's active
 * message interface. A message in VT must be derived from the type
 * vt::Message. The derived class will include the ``envelope'', which includes
 * the handler, and other information for processing the message
 */

//              VT Base Message
//            \----------------/
//             \              /
struct MyMsg : ::vt::Message {
  // In general, a default constructor is required for the a message because it
  // may be reconstructed by VT
  MyMsg() = default;

  // A normal constructor
  MyMsg(int in_a, int in_b) : a_(in_a), b_(in_b) { }

  int getA() const { return a_; }
  int getB() const { return b_; }

private:
  int a_ = 0, b_ = 0;
};


static inline void activeMessageNode() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();

  /*
   * All nodes will invoke this (see calling code) in a SPMD-style execution
   * reflective of the MPI style. After initialize, every rank executes this
   * function: activeMessageNode(). However, this is not required: each node can
   * execute any code the user wishes. Typically, a "root" node may kick off
   * some messages, while the other nodes just execute the scheduler code.
   *
   * A basic send essentially does an ``MPI_Send(..,destination,...)'' to the
   * destination node passed to ::vt::theMsg()->sendMsg(destination). The
   * handler, which is passed as the second template argument, is the function
   * that is triggered when the message arrives on the destination node.
   */

  if (this_node == 0) {
    auto msg = ::vt::makeMessage<MyMsg>(10*(this_node+1),20*(this_node+1));
    ::vt::theMsg()->sendMsg<MyMsg,msgHandlerA>(this_node+1, msg.get());
  }
}

static void msgHandlerA(MyMsg* msg) {

}

}} /* end namespace vt::tutorial */
