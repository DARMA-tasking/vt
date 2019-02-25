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

#include "test_location_common.h"

namespace vt { namespace tests { namespace unit {

// fixtures
struct TestLocation : TestParallelHarness {};
template <typename MsgT>
struct TestLocationRoute : TestLocation {};

TEST_F(TestLocation, test_register_and_get_entity) {

  auto const my_node = vt::theContext()->getNode();
  auto const home    = 0;
  auto const entity  = locat::arbitrary_entity;

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
    vt::theLocMan()->virtual_loc->unregisterEntity(entity);
  }
}

TEST_F(TestLocation, test_register_and_get_multiple_entities) {

  auto const my_node  = vt::theContext()->getNode();
  auto const nb_nodes = vt::theContext()->getNumNodes();
  auto const entity   = locat::arbitrary_entity + my_node;

  // Register the entity on the current node
  vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
  // Wait for every nodes to be registered
  vt::theCollective()->barrier();

  // Every nodes to a get location on every entity
  for (auto i = 0; i < nb_nodes; ++i) {
    bool success = false;
    // The entity can be located on the node where it has been registered
    vt::theLocMan()->virtual_loc->getLocation(
      locat::arbitrary_entity + i, i, [i, &success, my_node](vt::NodeType node) {

        auto const cur = vt::theContext()->getNode();
        // let p: i == my_node and q: cur == node
        // we have: p implies q == not(p) or q
        EXPECT_TRUE(i not_eq my_node or cur == node);
        EXPECT_EQ(i, node);
        success = true;
      }
    );
    if (i == my_node) {
      // this test can only be done for cases where getLocation is synchronous -> home_node
      EXPECT_TRUE(success);
    }
  }
  // finalize
  vt::theLocMan()->virtual_loc->unregisterEntity(entity);
}

TEST_F(TestLocation, test_unregister_multiple_entities) {

  auto const nb_nodes = vt::theContext()->getNumNodes();
  auto const my_node  = vt::theContext()->getNode();
  auto const entity   = locat::arbitrary_entity + my_node;

  vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
  vt::theLocMan()->virtual_loc->unregisterEntity(entity);

  // Wait for every nodes to be registered and unregister
  vt::theCollective()->barrier();

  for (auto i = 0; i < nb_nodes; ++i) {
    // The entity can be located on the node where it has been registered
    vt::theLocMan()->virtual_loc->getLocation(
      locat::arbitrary_entity + i, i, [](vt::NodeType node) {
        // This lambda should not be executed if the unregisterEntity works correctly
        FAIL() << "entity should have been yet unregistered";
      }
    );
  }
}

TEST_F(TestLocation, test_migrate_entity) {

  auto const nb_nodes = vt::theContext()->getNumNodes();
  auto const my_node  = vt::theContext()->getNode();
  auto const entity   = locat::arbitrary_entity;
  auto const old_home = 0;
  auto const new_home = 1;

  // Register the entity on the node 0
  if (my_node == old_home) {
    vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
  }

  bool done = false;
  vt::theLocMan()->virtual_loc->getLocation(entity, old_home, [old_home,&done](vt::NodeType node) {
    EXPECT_EQ(old_home, node);
    done = true;
  });

  while (not done) { vt::runScheduler(); }
  vt::theCollective()->barrier();

  if (my_node == old_home) {
    vt::theLocMan()->virtual_loc->entityMigrated(entity, new_home);
  } else if (my_node == new_home) {
    vt::theLocMan()->virtual_loc->registerEntityMigrated(entity, my_node);
  }

  vt::theCollective()->barrier();

  if (my_node > 1) {
    vt::theLocMan()->virtual_loc->getLocation(entity, old_home, [old_home,new_home](vt::NodeType node) {
      // Edit: the expected node can be either 0 (initial) or 1 (migrated)
      // The protocol may actually eagerly update other nodes
      EXPECT_TRUE(node == old_home or node == new_home);
    });
  }
  // finalize
  if (my_node == new_home) {
    vt::theLocMan()->virtual_loc->unregisterEntity(entity);
  }
}

TEST_F(TestLocation, test_migrate_multiple_entities) {

  auto const nb_nodes  = vt::theContext()->getNumNodes();
  auto const my_node   = vt::theContext()->getNode();
  auto const next_node = (my_node == nb_nodes - 1 ? 0 : my_node + 1);
  auto const entity    = locat::arbitrary_entity + my_node;

  // register the entity on the current node
  vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
  vt::theCollective()->barrier();

  // shift entity node to the right (modulo nb_nodes)
  vt::theLocMan()->virtual_loc->entityMigrated(entity, next_node);

  auto previous_node_entity = (
    my_node == 0 ?
      locat::arbitrary_entity + nb_nodes - 1 :
      locat::arbitrary_entity + my_node - 1
  );
  vt::theLocMan()->virtual_loc->registerEntityMigrated(previous_node_entity, my_node);
  vt::theCollective()->barrier();

  // Every nodes to a get location on every entity
  for (auto i = 0; i < nb_nodes; ++i) {
    bool success = false;
    // The entity can be located on the node where it has been registered
    vt::theLocMan()->virtual_loc->getLocation(
      locat::arbitrary_entity + i, i, [i, &success, my_node, nb_nodes](vt::NodeType node) {
        auto expected_node = (i + 1 < nb_nodes ? i + 1 : 0);
        EXPECT_EQ(expected_node, node);
        success = true;
      }
    );
    // this test can only be done for cases where getLocation is synchronous -> local or cache
    if (i == my_node || i + 1 == my_node) {
      EXPECT_TRUE(success);
    }
  }
  // finalize
  if (my_node == next_node) {
    vt::theLocMan()->virtual_loc->unregisterEntity(entity);
  }
}

TYPED_TEST_CASE_P(TestLocationRoute);
TYPED_TEST_P(TestLocationRoute, test_route_entity) {

  auto const nb_nodes = vt::theContext()->getNumNodes();
  auto const my_node  = vt::theContext()->getNode();
  auto const entity   = locat::arbitrary_entity;
  auto const home     = 0;

  // Register the entity on the node 0
  if (my_node == home) {
    int msg_count = 0;

    vt::theLocMan()->virtual_loc->registerEntity(
      entity, my_node, [&msg_count](vt::BaseMessage* raw_msg) {
        // count the number of routed messages received by the home node
        auto msg = static_cast<TypeParam*>(raw_msg);
        auto dst = vt::theContext()->getNode();
        debug_print(
          location, node,
          "rank:{} a message arrived for entity: {}\n", dst, msg->entity_
        );
        EXPECT_EQ(msg->entity_, locat::magic_number + msg->from_);
        msg_count++;
      }
    );

    // send long messages for entity
    bool is_long = std::is_same<TypeParam,locat::LongMsg>::value;
    auto msg = vt::makeSharedMessage<locat::EntityMsg>(entity, my_node, is_long);
    vt::theMsg()->broadcastMsg< locat::EntityMsg, locat::routeTestHandler<TypeParam> >(msg);

    while (msg_count < nb_nodes - 1) { vt::runScheduler(); }
    debug_print(
      location, node,
      "all messages have been arrived.\n"
    );

    EXPECT_EQ(msg_count, nb_nodes - 1);
  }
  // finalize
  if (my_node == home) {
    vt::theLocMan()->virtual_loc->unregisterEntity(entity);
  }
}

TYPED_TEST_P(TestLocationRoute, test_entity_cache_hits){

  auto const nb_nodes  = vt::theContext()->getNumNodes();
  auto const my_node   = vt::theContext()->getNode();
  auto const entity    = locat::arbitrary_entity;
  auto const home      = 0;
  auto const nb_rounds = 3;
  auto nb_received     = 0;

  // register entity and count received messages
  if (my_node == home) {
    vt::theLocMan()->virtual_loc->registerEntity(
      entity, my_node, [&](vt::BaseMessage* msg){ nb_received++; }
    );
  }

  // check cache consistency for the given entity
  locat::verifyCacheConsistency<TypeParam>(entity, my_node, home, home, nb_rounds);

  // finalize
  if (my_node == home) {
    vt::theLocMan()->virtual_loc->unregisterEntity(entity);
    EXPECT_EQ(nb_received, (nb_nodes - 1) * nb_rounds);
    EXPECT_FALSE(locat::isCached(entity));
  }
}

TYPED_TEST_P(TestLocationRoute, test_entity_cache_migrated_entity){

  auto const nb_nodes  = vt::theContext()->getNumNodes();
  auto const my_node   = vt::theContext()->getNode();
  auto const entity    = locat::arbitrary_entity;
  auto const home      = 0;
  auto const new_home  = 3;
  auto const nb_rounds = 3;
  auto nb_received     = 0;

  // register entity
  if (my_node == home) {
    vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
  }
  vt::theCollective()->barrier();

  if (my_node == home) {
    // migrate entity: unregister it but keep its id in cache
    vt::theLocMan()->virtual_loc->entityMigrated(entity, new_home);
    EXPECT_TRUE(locat::isCached(entity));
  } else if (my_node == new_home) {
    // receive migrated entity: register it and keep in cache
    vt::theLocMan()->virtual_loc->registerEntityMigrated(
      entity, my_node, [entity,&nb_received](vt::BaseMessage* in_msg) {
        debug_print(
          location, node,
          "rank:{}: a message arrived to me for a migrated entity {}.\n",
          vt::theContext()->getNode(), entity
        );
        nb_received++;
      }
    );
    EXPECT_TRUE(locat::isCached(entity));
  }

  // check cache consistency for the given entity
  locat::verifyCacheConsistency<TypeParam>(entity, my_node, home, new_home, nb_rounds);

    // finalize
  if (my_node == new_home) {
    vt::theLocMan()->virtual_loc->unregisterEntity(entity);
    auto const min_expected_ack = (nb_nodes - 2) * nb_rounds;
    EXPECT_TRUE(nb_received >= min_expected_ack);
    EXPECT_FALSE(locat::isCached(entity));
  }
}

REGISTER_TYPED_TEST_CASE_P(
  TestLocationRoute, test_route_entity, test_entity_cache_hits, test_entity_cache_migrated_entity
);
INSTANTIATE_TYPED_TEST_CASE_P(Message, TestLocationRoute, locat::MsgType);

}}} // end namespace vt::tests::unit