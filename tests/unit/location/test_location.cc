
#include "test_location_message.h"

namespace vt { namespace tests { namespace unit {


struct TestLocation : TestParallelHarness {};
// enable parameterized tests
template <typename T>
struct TestLocationRoute : TestParallelHarness {};

TEST_F(TestLocation, test_registering_and_get_entity) {

  auto const& my_node = vt::theContext()->getNode();

  int entity = locat::arbitrary_entity;

  // Register the entity on the node 0
  if (my_node == 0) {
    vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
  }

  bool success= false;
  vt::theLocMan()->virtual_loc->getLocation(entity, 0, [&success](vt::NodeType node) {
    EXPECT_EQ(0, node);
    success= true;
  });
  if (my_node == 0) {
    // this test can only be done for cases where getLocation is synchronous -> home_node
    EXPECT_TRUE(success);
  }
}

TEST_F(TestLocation, test_registering_and_get_entities) {

  auto const& my_node = vt::theContext()->getNode();


  int entity = locat::arbitrary_entity + my_node;

  // Register the entity on the current node
  vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);

  // Wait for every nodes to be registered
  vt::theCollective()->barrier();

  auto const& numNodes = vt::theContext()->getNumNodes();
  // Every nodes to a get location on every entity
  for (auto i = 0; i < numNodes; ++i) {
    bool success= false;
    // The entity can be located on the node where it has been registered
    vt::theLocMan()->virtual_loc->getLocation(
      locat::arbitrary_entity + i, i, [i, &success, my_node](vt::NodeType node) {
        // fmt::print("\n{}: test get location for ieme entity={} and find it on node={}
        // when my_node is={}\n", vt::theContext()->getNode(), i, node, my_node);
        if (i == my_node) {
          EXPECT_EQ(vt::theContext()->getNode(), node);
        } else {
          EXPECT_TRUE(vt::theContext()->getNode() != node);
        }
        EXPECT_EQ(i, node);
        success= true;
      }
    );
    if (i == my_node) {
      // this test can only be done for cases where getLocation is synchronous -> home_node
      EXPECT_TRUE(success);
    }
  }
}

TEST_F(TestLocation, test_unregistering_entities) {

  auto const& my_node = vt::theContext()->getNode();


  int entity = locat::arbitrary_entity + my_node;
  vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
  vt::theLocMan()->virtual_loc->unregisterEntity(entity);

  // Wait for every nodes to be registered and unregister
  vt::theCollective()->barrier();

  auto const& numNodes = vt::theContext()->getNumNodes();
  // Every nodes to a get location on every entity
  for (auto i = 0; i < numNodes; ++i) {
    bool success= false;
    // The entity can be located on the node where it has been registered
    vt::theLocMan()->virtual_loc->getLocation(
      locat::arbitrary_entity + i, i, [i, &success, my_node](vt::NodeType node) {
        // This lambda should not be executed if the unregisterEntity works correclty
        EXPECT_TRUE(false);
      }
    );
  }
}

TEST_F(TestLocation, test_migration_entity) {

  auto const& numNodes = vt::theContext()->getNumNodes();
  if (numNodes > 1) {


    auto const& my_node = vt::theContext()->getNode();

    int entity = locat::arbitrary_entity;

    bool done = false;

    // Register the entity on the node 0
    if (my_node == 0) {
      vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
    }

    vt::theLocMan()->virtual_loc->getLocation(entity, 0, [my_node,&done](vt::NodeType node) {
      EXPECT_EQ(0, node);
      done = true;
    });


    while (!done) { vt::runScheduler(); }

    vt::theCollective()->barrier();

    if (my_node == 0) {
      vt::theLocMan()->virtual_loc->entityMigrated(entity, 1);
    } else if (my_node == 1) {
      vt::theLocMan()->virtual_loc->registerEntityMigrated(entity, my_node);
    }

    vt::theCollective()->barrier();

    if (my_node > 1) {
      vt::theLocMan()->virtual_loc->getLocation(entity, 0, [my_node](vt::NodeType node) {
        // Edit: the expected node can be either 0 (initial) or 1 (migrated)
        // The protocol may actually eagerly update other nodes
        EXPECT_TRUE(node == 0 or node == 1);
      });
    }
  }
}

TEST_F(TestLocation, test_migration_entities) {

  auto const& numNodes = vt::theContext()->getNumNodes();
  if (numNodes > 1) {


    auto const& my_node = vt::theContext()->getNode();

    int entity = locat::arbitrary_entity + my_node;

    // Register the entity on the node 0
    vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);

    vt::theCollective()->barrier();

    // migrate every nodes from n to n + 1 except for the lat one which move to 0
    auto next_node = my_node == numNodes - 1 ? 0 : my_node + 1;
    vt::theLocMan()->virtual_loc->entityMigrated(entity, next_node);

    auto previous_node_entity = my_node == 0 ?
      locat::arbitrary_entity + numNodes - 1 :
      locat::arbitrary_entity + my_node - 1;
    vt::theLocMan()->virtual_loc->registerEntityMigrated(previous_node_entity, my_node);

    vt::theCollective()->barrier();

    auto const& numNodes = vt::theContext()->getNumNodes();
    // Every nodes to a get location on every entity
    for (auto i = 0; i < numNodes; ++i) {
      bool success= false;
      // The entity can be located on the node where it has been registered
      vt::theLocMan()->virtual_loc->getLocation(
        locat::arbitrary_entity + i, i, [i, &success, my_node, numNodes](vt::NodeType node) {
          auto expectedNode = i + 1 < numNodes ? i + 1 : 0;
          EXPECT_EQ(expectedNode, node);
          success = true;
        }
      );
      // this test can only be done for cases where getLocation is synchronous -> local or cache
      if (i == my_node || i + 1 == my_node) {
        EXPECT_TRUE(success);
      }
    }
  }
}

TYPED_TEST_CASE_P(TestLocationRoute);
TYPED_TEST_P(TestLocationRoute, test_route_message) {

  auto const& my_node = vt::theContext()->getNode();
  auto const& numNodes = vt::theContext()->getNumNodes();

  int entity = locat::arbitrary_entity;

  // Register the entity on the node 0
  if (my_node == 0) {
    int msg_count = 0;
    // Associate a message_action to the entity that is triggered when a message arrives for the entity
    vt::theLocMan()->virtual_loc->registerEntity(
      entity,
      my_node,
      [&msg_count](BaseMessage* in_msg) {
        auto msg = static_cast<TypeParam*>(in_msg);
        auto dst = vt::theContext()->getNode();
        fmt::print("{}: a message arrived with data: {}.\n",dst, msg->data_);
        EXPECT_EQ(msg->data_, locat::magic_number + msg->from_);
        msg_count++;
      }
    );

    // send long messages for entity
    bool is_long = std::is_same<TypeParam,locat::MyLongTestMsg>::value;
    auto msg = vt::makeSharedMessage<locat::EntityMsg>(entity, my_node, is_long);

    vt::theMsg()->broadcastMsg<locat::EntityMsg, locat::entityTestHandler>(msg);

    while (msg_count < numNodes - 1) { vt::runScheduler(); }

    fmt::print("all messages have been arrived.\n");
    EXPECT_EQ(msg_count, numNodes - 1);
  }
}

using MsgTypes = testing::Types<locat::MyShortTestMsg, locat::MyLongTestMsg>;

REGISTER_TYPED_TEST_CASE_P(TestLocationRoute, test_route_message);
INSTANTIATE_TYPED_TEST_CASE_P(Message, TestLocationRoute, MsgTypes);

}}} // end namespace vt::tests::unit