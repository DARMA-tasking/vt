/*
//@HEADER
// ************************************************************************
//
//                        test_location.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include "test_location_message.h"

namespace vt { namespace tests { namespace unit {

// fixtures
struct TestLocation : TestParallelHarness {};

template <typename MsgT>
struct TestLocationRoute : TestLocation {};

TEST_F(TestLocation, test_registering_and_get_entity) {

  vt::NodeType const home = 0;
  auto const my_node = vt::theContext()->getNode();
  int entity = locat::arbitrary_entity;

  // Register the entity on the node 0
  if (my_node == home) {
    vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
  }

  bool success = false;
  vt::theLocMan()->virtual_loc->getLocation(
    entity, home, [home,&success](vt::NodeType node) {
      EXPECT_EQ(home, node);
      success = true;
    }
  );

  if (my_node == home) {
    // only performed if getLocation is synchronous, i.e on home_node
    EXPECT_TRUE(success);
  }
}


TEST_F(TestLocation, test_registering_and_get_entities) {

  auto const& my_node  = vt::theContext()->getNode();
  auto const& numNodes = vt::theContext()->getNumNodes();

  int entity = locat::arbitrary_entity + my_node;
  // Register the entity on the current node
  vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
  // Wait for every nodes to be registered
  vt::theCollective()->barrier();

  // Every nodes to a get location on every entity
  for (auto i = 0; i < numNodes; ++i) {
    bool success= false;
    // The entity can be located on the node where it has been registered
    vt::theLocMan()->virtual_loc->getLocation(
      locat::arbitrary_entity + i, i, [i, &success, my_node](vt::NodeType node) {
        auto cur = vt::theContext()->getNode();
        if (i == my_node) {
          EXPECT_EQ(cur, node);
        } else {
          EXPECT_TRUE(cur not_eq node);
        }
        EXPECT_EQ(i, node);
        success = true;
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
        // This lambda should not be executed if the unregisterEntity works correctly
        FAIL() << "entity should have been yet unregistered";
      }
    );
  }
}

TEST_F(TestLocation, test_migration_entity) {

  auto const& numNodes = vt::theContext()->getNumNodes();
  vtAssertExpr(numNodes > 1);

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

TEST_F(TestLocation, test_migration_entities) {

  auto const& numNodes = vt::theContext()->getNumNodes();
  vtAssertExpr(numNodes > 1);

  auto const& my_node = vt::theContext()->getNode();
  int entity = locat::arbitrary_entity + my_node;

  // Register the entity on the node 0
  vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
  vt::theCollective()->barrier();

  // migrate every nodes from n to n + 1 except for the lat one which move to 0
  auto next_node = my_node == numNodes - 1 ? 0 : my_node + 1;
  vt::theLocMan()->virtual_loc->entityMigrated(entity, next_node);

  auto previous_node_entity = (
    my_node == 0 ?
      locat::arbitrary_entity + numNodes - 1 :
      locat::arbitrary_entity + my_node - 1
  );
  vt::theLocMan()->virtual_loc->registerEntityMigrated(previous_node_entity, my_node);

  vt::theCollective()->barrier();

  // Every nodes to a get location on every entity
  for (auto i = 0; i < numNodes; ++i) {
    bool success = false;
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
      entity, my_node, [&msg_count](vt::BaseMessage* in_msg) {

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

    vt::theMsg()->broadcastMsg< locat::EntityMsg, locat::entityTestHandler<TypeParam> >(msg);

    while (msg_count < numNodes - 1) { vt::runScheduler(); }

    fmt::print("all messages have been arrived.\n");
    EXPECT_EQ(msg_count, numNodes - 1);
  }
}

REGISTER_TYPED_TEST_CASE_P(TestLocationRoute, test_route_message);
INSTANTIATE_TYPED_TEST_CASE_P(Message, TestLocationRoute, locat::MsgType);

}}} // end namespace vt::tests::unit