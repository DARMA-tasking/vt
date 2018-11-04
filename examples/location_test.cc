
#include "vt/transport.h"
#include <cstdlib>

using namespace vt;

using EntityType = int32_t;

struct EntityMsg : vt::Message {
  EntityType entity;
  NodeType home;

  EntityMsg(EntityType const& in_entity, NodeType const& in_home)
    : Message(), entity(in_entity), home(in_home)
  { }
};

static void entityTestHandler(EntityMsg* msg) {
  fmt::print(
    "{}: entityTestHandler entity={}\n", theContext()->getNode(), msg->entity
  );

  theLocMan()->virtual_loc->getLocation(msg->entity, msg->home, [](NodeType node){
    fmt::print("{}: entityTestHandler: location={}\n", theContext()->getNode(), node);
    theLocMan()->virtual_loc->printCurrentCache();
  });
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  int32_t entity = 10, entity2 = 11, entity3 = 12;

  if (my_node == 0) {
    theLocMan()->virtual_loc->registerEntity(entity,my_node);
    theLocMan()->virtual_loc->registerEntity(entity2,my_node);
  } else if (my_node == 1) {
    theLocMan()->virtual_loc->registerEntity(entity3,my_node);
  }

  if (my_node == 0) {
    theMsg()->broadcastMsg<EntityMsg, entityTestHandler>(
      makeSharedMessage<EntityMsg>(entity, my_node)
    );
    theMsg()->broadcastMsg<EntityMsg, entityTestHandler>(
      makeSharedMessage<EntityMsg>(entity2, my_node)
    );
  } else if (my_node == 1) {
    theMsg()->broadcastMsg<EntityMsg, entityTestHandler>(
      makeSharedMessage<EntityMsg>(entity3, my_node)
    );
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
