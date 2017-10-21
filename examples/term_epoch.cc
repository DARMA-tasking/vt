
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

  PropagateMsg(NodeType const& in_from)
    : Message(), from(in_from), life_time(drand48() * max_life_time)
  { }

  PropagateMsg(NodeType const& in_from, TTLType const& in_ttl)
    : Message(), from(in_from), life_time(in_ttl)
  { }
};

static void sendMsgEpoch(EpochType const& epoch, TTLType const& ttl = no_ttl);

static void propagate_handler(PropagateMsg* msg) {
  EpochType const epoch = envelopeGetEpoch(msg->env);
  printf("%d: propagate_handler: msg=%p, epoch=%d\n", my_node, msg, epoch);
  if (msg->life_time > 0) {
    sendMsgEpoch(epoch, msg->life_time);
  }
}

static void sendMsgEpoch(EpochType const& epoch, TTLType const& ttl) {
  NodeType const random_node = drand48() * num_nodes;
  PropagateMsg* msg = nullptr;

  if (ttl == no_ttl) {
    // generate a random TLL for the new msg
    msg = makeSharedMessage<PropagateMsg>(my_node);
  } else {
    msg = makeSharedMessage<PropagateMsg>(my_node, ttl - 1);
  }

  if (epoch != no_epoch) {
    theMsg()->setEpochMessage(msg, epoch);
  }

  printf("%d: sending msg: epoch=%d, ttl=%d\n", my_node, epoch, msg->life_time);

  theMsg()->sendMsg<PropagateMsg, propagate_handler>(random_node, msg);

  printf(
    "%d: sendMsgEpoch: epoch=%d, node=%d, ttl-%d\n",
    my_node, epoch, random_node, ttl
  );
}

static void sendStartEpoch(EpochType const& epoch) {
  int32_t const num_msgs = drand48() * max_msgs_per_node;
  for (int i = 0; i < num_msgs; i++) {
    sendMsgEpoch(epoch);
  }
}

static void next_epoch() {
  printf("%d: cur_epoch=%d\n", my_node, cur_epoch);

  if (use_epoch) {
    cur_epoch = theTerm()->newEpoch();

    if (cur_epoch < max_epochs) {
      printf("%d: new cur_epoch=%d\n", my_node, cur_epoch);

      sendStartEpoch(cur_epoch);

      theTerm()->attachEpochTermAction(cur_epoch, []{
        printf("%d: running attached action: cur_epoch=%d\n", my_node, cur_epoch);
        next_epoch();
      });
    }
  } else {
    sendStartEpoch(no_epoch);
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (argc > 1) {
    use_epoch = atoi(argv[1]) == 1 ? true : false;
  }

  printf("%d:use_epoch=%s\n", my_node, print_bool(use_epoch));

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  next_epoch();

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
