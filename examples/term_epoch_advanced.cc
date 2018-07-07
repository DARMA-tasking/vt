
#include "transport.h"

#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static EpochType cur_epoch = no_epoch;

using TTLType = int32_t;

static constexpr EpochType const max_epochs = 5;
static constexpr TTLType const max_life_time = 5;
static constexpr TTLType const no_ttl = -1000;
static constexpr int32_t const max_msgs_per_node = 5;

static bool use_epoch = true;

struct PropagateMsg : vt::Message {
  NodeType from = uninitialized_destination;
  TTLType life_time = no_ttl;

  explicit PropagateMsg(NodeType const& in_from)
    : ::vt::Message(), from(in_from), life_time(drand48() * max_life_time)
  { }

  PropagateMsg(NodeType const& in_from, TTLType const& in_ttl)
    : ::vt::Message(), from(in_from), life_time(in_ttl)
  { }
};

static void sendMsgEpoch(
  EpochType const& epoch, bool const set_epoch = false,
  TTLType const& ttl = no_ttl
);

static void propagateHandler(PropagateMsg* msg) {
  EpochType const epoch = envelopeGetEpoch(msg->env);
  fmt::print(
    "{}: propagate_handler: msg={}, epoch={}\n", my_node, print_ptr(msg), epoch
  );
  if (msg->life_time > 0) {
    sendMsgEpoch(epoch, false, msg->life_time);
  }
}

static void sendMsgEpoch(
  EpochType const& epoch, bool const set_epoch, TTLType const& ttl
) {
  NodeType const random_node = drand48() * num_nodes;
  PropagateMsg* msg = nullptr;

  if (ttl == no_ttl) {
    // generate a random TLL for the new msg
    msg = makeSharedMessage<PropagateMsg>(my_node);
  } else {
    msg = makeSharedMessage<PropagateMsg>(my_node, ttl - 1);
  }

  if (epoch != no_epoch && set_epoch) {
    theMsg()->setEpochMessage(msg, epoch);
  }

  fmt::print("{}: sending msg: epoch={}, ttl={}\n", my_node, epoch, msg->life_time);

  theMsg()->sendMsg<PropagateMsg, propagateHandler>(random_node, msg);

  fmt::print(
    "{}: sendMsgEpoch: epoch={}, node={}, ttl-{}\n",
    my_node, epoch, random_node, ttl
  );
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  if (my_node == 1) {
    auto const& rooted_new_epoch = theTerm()->makeEpochRooted();
    ::fmt::print("{}: new epoch={}\n", my_node, rooted_new_epoch);
    sendMsgEpoch(rooted_new_epoch, true, 5);
    theTerm()->activateEpoch(rooted_new_epoch);
    theTerm()->addAction(rooted_new_epoch, [=]{
      ::fmt::print("{}: epoch={}, action\n", my_node, rooted_new_epoch);
    });
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
