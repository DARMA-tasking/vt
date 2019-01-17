
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

using EntityType = int32_t;

struct EntityMsg : vt::Message {
  EntityType entity;
  NodeType home;

  EntityMsg(EntityType const& in_entity, NodeType const& in_home)
    : Message(), entity(in_entity), home(in_home)
  { }
};

class TestLocation : public TestParallelHarness {
};

TEST_F(TestLocation, test_registering_and_get_entity) {

  auto const& my_node = theContext()->getNode();
  using namespace vt;

  int32_t entity = 10;

  // Register the entity on the node 0
  if(my_node == 0)
  {
    theLocMan()->virtual_loc->registerEntity(entity, my_node);
  }

  bool succes= false;
  theLocMan()->virtual_loc->getLocation(entity, 0, [&succes](NodeType node){
    EXPECT_EQ(0, node);
  });
  succes= true;
  if(my_node == 0){
    // this test can only be done for cases where getLocation is synchronous -> home_node
    EXPECT_TRUE(succes);
  }
}

TEST_F(TestLocation, test_registering_and_get_entities) {

  auto const& my_node = theContext()->getNode();
  using namespace vt;

  int32_t entity = 10 + my_node;

  // Register the entity on the current node
  theLocMan()->virtual_loc->registerEntity(entity, my_node);

  // Wait for every nodes to be registered
  theCollective()->barrier();

  auto const& numNodes = theContext()->getNumNodes();
  // Every nodes to a get location on every entity
  for(auto i = 0; i < numNodes; ++i) {
    bool succes= false;
    // The entity can be located on the node where it has been registered
    theLocMan()->virtual_loc->getLocation(10 + i, i, [i, &succes, my_node](NodeType node){
      // fmt::print("\n{}: test get location for ieme entity={} and find it on node={} when my_node is={}\n", theContext()->getNode(), i, node, my_node);
      if(i == my_node) {
        EXPECT_EQ(theContext()->getNode(), node);
      }
      else {
        EXPECT_TRUE(theContext()->getNode() != node);
      }
      EXPECT_EQ(i, node);
      succes= true;
    });
    if(i == my_node) {
      // this test can only be done for cases where getLocation is synchronous -> home_node
      EXPECT_TRUE(succes);
    }
  }
}

TEST_F(TestLocation, test_unregistering_entities) {

  auto const& my_node = theContext()->getNode();
  using namespace vt;

  int32_t entity = 10 + my_node;
  theLocMan()->virtual_loc->registerEntity(entity, my_node);
  theLocMan()->virtual_loc->unregisterEntity(entity);

  // Wait for every nodes to be registered and unregister
  theCollective()->barrier();

  auto const& numNodes = theContext()->getNumNodes();
  // Every nodes to a get location on every entity
  for(auto i = 0; i < numNodes; ++i) {
    bool succes= false;
    // The entity can be located on the node where it has been registered
    theLocMan()->virtual_loc->getLocation(10 + i, i, [i, &succes, my_node](NodeType node){
      // This lambda should not be executed if the unregisterEntity works correclty
      EXPECT_TRUE(false);
    });
  }
}

TEST_F(TestLocation, test_migration_entity) {

  auto const& numNodes = theContext()->getNumNodes();
  if(numNodes > 1) {
    using namespace vt;

    auto const& my_node = theContext()->getNode();

    int32_t entity = 10;

    bool done = false;

    // Register the entity on the node 0
    if(my_node == 0)
    {
      theLocMan()->virtual_loc->registerEntity(entity, my_node);
    }

    theLocMan()->virtual_loc->getLocation(entity, 0, [my_node,&done](NodeType node){
      EXPECT_EQ(0, node);
      done = true;
    });


    while (!done) { vt::runScheduler(); }

    theCollective()->barrier();

    if(my_node == 0)
    {
      theLocMan()->virtual_loc->entityMigrated(entity, 1);
    } else if (my_node == 1) {
      theLocMan()->virtual_loc->registerEntityMigrated(entity, my_node);
    }

    theCollective()->barrier();

    if(my_node > 1) {
      theLocMan()->virtual_loc->getLocation(entity, 0, [my_node](NodeType node){
        // fmt::print("this node={}: node={}\n", my_node, node);
        // If the 0 node isn't the good one, it's the expected one because it is also the cached one
        // A routMsg has been send to the last know node (the 0) one which is guarenty to know the correct node)
        EXPECT_EQ(0, node);
      });
    }
    else {
      theLocMan()->virtual_loc->getLocation(entity, 0, [my_node](NodeType node){
        // fmt::print("this node={}: node={}\n", my_node, node);
        EXPECT_EQ(1, node);
      });
    }
  }
}

TEST_F(TestLocation, test_migration_entities) {

  auto const& numNodes = theContext()->getNumNodes();
  if(numNodes > 1) {
    using namespace vt;

    auto const& my_node = theContext()->getNode();

    int32_t entity = 10 + my_node;

    // Register the entity on the node 0

    theLocMan()->virtual_loc->registerEntity(entity, my_node);

    theCollective()->barrier();

    // migrate every nodes from n to n + 1 except for the lat one which move to 0
    if(my_node < numNodes - 1){
      theLocMan()->virtual_loc->entityMigrated(entity, my_node + 1);
    }
    else {
      theLocMan()->virtual_loc->entityMigrated(entity, 0);
    }

    if(my_node == 0){
      theLocMan()->virtual_loc->registerEntityMigrated(10 + numNodes - 1, my_node);
    }
    else{
      theLocMan()->virtual_loc->registerEntityMigrated(10 + my_node - 1, my_node);
    }

    theCollective()->barrier();

    auto const& numNodes = theContext()->getNumNodes();
    // Every nodes to a get location on every entity
    for(auto i = 0; i < numNodes; ++i) {
      bool succes= false;
      // The entity can be located on the node where it has been registered
      theLocMan()->virtual_loc->getLocation(10 + i, i, [i, &succes, my_node, numNodes](NodeType node){
        if(i + 1 < numNodes)
        {
          EXPECT_EQ(i + 1, node);
        }
        else
        {
          EXPECT_EQ(0, node);
        }

        succes= true;
      });
      // this test can only be done for cases where getLocation is synchronous -> local or cache
      if(i == my_node ||  i + 1 == my_node){
        EXPECT_TRUE(succes);
      }
    }
  }
}


}}} // end namespace vt::tests::unit
