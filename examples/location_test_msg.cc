
#include <cstdlib>
#include <cassert>
#include <cstdio>

#include "transport.h"

using namespace vt;

using EntityType = int32_t;
static constexpr EntityType const arbitrary_entity_id = 10;

static constexpr int const magic_number = 29;

struct EntityMsg : Message {
  EntityType entity;
  NodeType home;

  EntityMsg(EntityType const& in_entity, NodeType const& in_home)
    : Message(), entity(in_entity), home(in_home)
  { }
};

struct MyTestMsg : LocationRoutedMsg<EntityType, ShortMessage> {
  NodeType from_node = uninitialized_destination;
  int data = 0;

  MyTestMsg(int const& in_data, NodeType const& in_from_node)
    : LocationRoutedMsg<EntityType,ShortMessage>(), data(in_data),
    from_node(in_from_node)
  { }
};

static void entityTestHandler(EntityMsg* msg) {
  auto const& node = theContext()->getNode();

  printf(
    "%d: entityTestHandler entity=%d\n", node, msg->entity
  );

  MyTestMsg* test_msg = new MyTestMsg(magic_number + node, node);
  theLocMan()->virtual_loc->routeMsg(
    msg->entity, msg->home, test_msg, [=]{ delete test_msg; }
  );
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  EntityType entity = arbitrary_entity_id;

  if (my_node == 0) {
    theLocMan()->virtual_loc->registerEntity(entity, [](BaseMessage* in_msg){
      auto msg = static_cast<MyTestMsg*>(in_msg);

      assert(
        msg->data == magic_number + msg->from_node and
        "Message data is corrupted"
      );

      printf(
        "%d: handler triggered for test msg: data=%d\n",
        theContext()->getNode(), msg->data
      );
    });

    theMsg()->broadcastMsg<EntityMsg, entityTestHandler>(
      makeSharedMessage<EntityMsg>(entity, my_node)
    );
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
