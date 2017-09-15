
#include "transport.h"
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
  printf(
    "%d: entityTestHandler entity=%d\n", theContext->getNode(), msg->entity
  );

  theLocMan->virtual_loc->getLocation(msg->entity, msg->home, [](NodeType node){
    printf("%d: entityTestHandler: location=%d\n", theContext->getNode(), node);
    theLocMan->virtual_loc->printCurrentCache();
  });
}

int main(int argc, char** argv) {
  CollectiveOps::initializeContext(argc, argv);
  CollectiveOps::initializeRuntime();

  auto const& my_node = theContext->getNode();
  auto const& num_nodes = theContext->getNumNodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  int32_t entity = 10, entity2 = 11, entity3 = 12;

  if (my_node == 0) {
    theLocMan->virtual_loc->registerEntity(entity);
    theLocMan->virtual_loc->registerEntity(entity2);
  } else if (my_node == 1) {
    theLocMan->virtual_loc->registerEntity(entity3);
  }

  if (my_node == 0) {
    theMsg->broadcastMsg<EntityMsg, entityTestHandler>(
      makeSharedMessage<EntityMsg>(entity, my_node)
    );
    theMsg->broadcastMsg<EntityMsg, entityTestHandler>(
      makeSharedMessage<EntityMsg>(entity2, my_node)
    );
  } else if (my_node == 1) {
    theMsg->broadcastMsg<EntityMsg, entityTestHandler>(
      makeSharedMessage<EntityMsg>(entity3, my_node)
    );
  }

  while (1) {
    runScheduler();
  }

  return 0;
}
