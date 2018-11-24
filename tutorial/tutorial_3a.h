
#include "vt/transport.h"

namespace vt { namespace tutorial {

//              VT Base Message
//             \----------------/
//              \              /
struct ExampleMsg : ::vt::Message {
  ExampleMsg() = default;
  explicit ExampleMsg(int32_t in_ttl) : ttl(in_ttl-1) { }

  int32_t ttl = 0;
};

// Forward declaration for the active message handler
static void recurHandler(ExampleMsg* msg);

// Tutorial code to demonstrate using a callback
static inline void activeMessageTerm() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();

  /*
   * Termination will allow us to track a subcomputation with causality to
   * determine when a sub-computation terminated in a distributed-manner. The
   * tutorial demonstrates how to use `collective` epochs. Rooted epoch will be
   * demonstrated in a follow-on tutorial.
   */

  // Create a new epoch: this is a collective invocation
  auto const new_epoch = theTerm()->newEpoch();

  if (this_node == 0) {
    auto msg = vt::makeSharedMessage<ExampleMsg>(8);
    envelopeSetEpoch(msg->env, new_epoch);
    vt::theMsg()->sendMsg<ExampleMsg,recurHandler>(this_node+1,msg);
  }

  // Any node that wishes to have a notification on termination for a given
  // epoch can add actions for the termination detector
  theTerm()->addAction(
    new_epoch, []{
      auto const node = vt::theContext()->getNode();
      fmt::print("{}: recurHandler terminated\n", node);
    }
  );

  // This is not explicitly a collective, but all nodes need to call
  // `finishedEpoch` to tell the system they are finished sending messages
  // for the epoch.
  theTerm()->finishedEpoch(new_epoch);
}

// Message handler that recursively sends messages
static void recurHandler(ExampleMsg* msg) {
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();
  NodeType const this_node = ::vt::theContext()->getNode();

  ::fmt::print(
    "{}: recurHandler: ttl={}, triggered\n", this_node, msg->ttl
  );

  if (msg->ttl > 0) {
    auto const num_send = static_cast<int32_t>(drand48() * 3);
    for (auto i = 0; i < num_send; i++) {
      auto next_node = (this_node + 1 > num_nodes - 1) ? 0 : (this_node + 1);

      ::fmt::print(
        "{}: recurHandler: i={}, next_node={}, num_send={}\n",
        this_node, i, next_node, num_send
      );

      auto msg_send = vt::makeSharedMessage<ExampleMsg>(msg->ttl);
      vt::theMsg()->sendMsg<ExampleMsg,recurHandler>(next_node,msg_send);
    }
  }
}

}} /* end namespace vt::tutorial */
