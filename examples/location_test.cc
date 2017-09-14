
#include "transport.h"
#include <cstdlib>

using namespace vt;

using EntityType = int32_t;

struct EntityMsg : vt::Message {
  EntityType entity;

  EntityMsg(int const& in_entity)
    : Message(), entity(in_entity)
  { }
};

static void entityTestHandler(EntityMsg* msg) {
  printf(
    "%d: entityTestHandler entity=%d\n", theContext->getNode(), msg->entity
  );

  theLocMan->virtual_loc->getLocation(msg->entity, 0, [](NodeType node){
    printf("%d: entityTestHandler: location=%d\n", theContext->getNode(), node);
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

  int32_t entity = 10;

  if (my_node == 0) {
    theLocMan->virtual_loc->registerEntity(entity);
  }

  if (my_node == 0) {
    EntityMsg* msg = new EntityMsg(entity);
    theMsg->broadcastMsg<EntityMsg, entityTestHandler>(msg, [=]{ delete msg; });
  }

  while (1) {
    runScheduler();
  }

  return 0;
}
