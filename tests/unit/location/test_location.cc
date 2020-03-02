/*
//@HEADER
// *****************************************************************************
//
//                               test_location.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include "test_location_common.h"

namespace vt { namespace tests { namespace unit {

// fixtures
struct TestLocation : TestParallelHarness {};
template <typename MsgT>
struct TestLocationRoute : TestLocation {};

TEST_F(TestLocation, test_register_and_get_entity) /* NOLINT */ {

  auto const my_node = vt::theContext()->getNode();
  auto const home    = 0;
  auto const entity  = location::default_entity;

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

  while (not success) {
    vt::runScheduler();
  }

  vt::theCollective()->barrier();

  if (my_node == home) {
    EXPECT_TRUE(success);
  }
}

TEST_F(TestLocation, test_register_and_get_multiple_entities)  /* NOLINT */ {

  auto const my_node  = vt::theContext()->getNode();
  auto const nb_nodes = vt::theContext()->getNumNodes();
  auto const entity   = location::default_entity + my_node;

  // Register the entity on the current node
  vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
  // Wait for every nodes to be registered
  vt::theCollective()->barrier();

  int check_sum = 0;
  bool success;

  for (auto i = 0; i < nb_nodes; ++i) {
    success = false;
    // The entity can be located on the node where it has been registered
    vt::theLocMan()->virtual_loc->getLocation(
      location::default_entity + i, i,
      [i, &success, &check_sum, my_node](vt::NodeType node) {
        auto const cur = vt::theContext()->getNode();
        // let p: i == my_node and q: cur == node
        // we have: p implies q == not(p) or q
        EXPECT_TRUE(i != my_node or cur == node);
        EXPECT_EQ(i, node);
        success = true;
        check_sum++;
      }
    );

    while (not success) {
      vt::runScheduler();
    }

    vt::theCollective()->barrier();

    if (i == my_node) {
      EXPECT_TRUE(success);
      EXPECT_EQ(check_sum, i + 1);
    }
  }
}

TEST_F(TestLocation, test_unregister_multiple_entities) /* NOLINT */ {

  auto const nb_nodes = vt::theContext()->getNumNodes();
  auto const my_node  = vt::theContext()->getNode();
  auto const entity   = location::default_entity + my_node;

  vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
  vt::theLocMan()->virtual_loc->unregisterEntity(entity);

  // Wait for every nodes to be registered and unregister
  vt::theCollective()->barrier();

  for (auto i = 0; i < nb_nodes; ++i) {
    // The entity can be located on the node where it has been registered
    vt::theLocMan()->virtual_loc->getLocation(
      location::default_entity + i, i, [](vt::NodeType node) {
        // This lambda should not be executed if the unregisterEntity works
        // correctly
        FAIL() << "entity should have been yet unregistered";
      }
    );
  }
}

TEST_F(TestLocation, test_migrate_entity) /* NOLINT */ {

  auto const nb_nodes = vt::theContext()->getNumNodes();

  // cannot perform entity migration if less than 3 nodes
  if (nb_nodes > 2) {
    auto const my_node  = vt::theContext()->getNode();
    auto const entity   = location::default_entity;
    auto const old_home = 0;
    auto const new_home = 1;

    // Register the entity on the node 0
    if (my_node == old_home) {
      vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
    }

    bool done = false;
    vt::theLocMan()->virtual_loc->getLocation(
      entity, old_home, [old_home,&done](vt::NodeType node) {
        EXPECT_EQ(old_home, node);
        done = true;
      }
    );

    while (not done) {
      vt::runScheduler();
    }

    vt::theCollective()->barrier();

    if (my_node == old_home) {
      vt::theLocMan()->virtual_loc->entityMigrated(entity, new_home);
    } else if (my_node == new_home) {
      vt::theLocMan()->virtual_loc->registerEntityMigrated(entity, my_node);
    }

    vt::theCollective()->barrier();

    if (my_node > 1) {
      vt::theLocMan()->virtual_loc->getLocation(
        entity, old_home, [=](vt::NodeType node) {
          // Edit: the expected node can be either 0 (initial) or 1 (migrated)
          // The protocol may actually eagerly update other nodes
          EXPECT_TRUE(node == old_home or node == new_home);
        }
      );
    }
  }
}

TEST_F(TestLocation, test_migrate_multiple_entities) /* NOLINT */ {

  auto const nb_nodes = vt::theContext()->getNumNodes();

  // cannot perform entity migration if less than 3 nodes
  if (nb_nodes > 2) {
    auto const my_node   = vt::theContext()->getNode();
    auto const next_node = (my_node + 1) % nb_nodes;
    auto const entity    = location::default_entity + my_node;

    // register the entity on the current node
    vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
    vt::theCollective()->barrier();

    // shift entity node to the right (modulo nb_nodes)
    vt::theLocMan()->virtual_loc->entityMigrated(entity, next_node);

    auto prev_node = (
      my_node == 0 ?
      location::default_entity + nb_nodes - 1 :
      location::default_entity + my_node - 1
    );
    vt::theLocMan()->virtual_loc->registerEntityMigrated(prev_node, my_node);
    vt::theCollective()->barrier();

    int check_sum = 0;
    bool success;

    for (auto i = 0; i < nb_nodes; ++i) {
      success = false;
      // The entity can be located on the node where it has been registered
      vt::theLocMan()->virtual_loc->getLocation(
        location::default_entity + i, i,
        [i, &success, &check_sum, nb_nodes](vt::NodeType found) {

          auto expected = (i + 1) % nb_nodes;
          EXPECT_EQ(expected, found);
          success = true;
          check_sum++;

          debug_print(
            location, node,
            "TestLocation: get loc migrated entity={}: found={}, expected={}\n",
            location::default_entity + i, found, expected
          );
        }
      );

      while (not success) {
        vt::runScheduler();
      }

      vt::theCollective()->barrier();

      // this test can only be done for cases where getLocation is synchronous ->
      // local or cache
      if (i == my_node || i + 1 == my_node) {
        EXPECT_TRUE(success);
        EXPECT_EQ(check_sum, i + 1);
      }
    }
  }
}

TYPED_TEST_SUITE_P(TestLocationRoute);

TYPED_TEST_P(TestLocationRoute, test_route_entity) {

  auto const nb_nodes = vt::theContext()->getNumNodes();
  auto const my_node  = vt::theContext()->getNode();
  auto const entity   = location::default_entity;
  auto const home     = 0;

  // Register the entity on the node 0
  if (my_node == home) {
    int msg_count = 0;

    vt::theLocMan()->virtual_loc->registerEntity(
      entity, my_node, [&msg_count](vt::BaseMessage* raw_msg) {
        // count the number of routed messages received by the home node
        auto msg = static_cast<TypeParam*>(raw_msg);
        debug_print(
          location, node,
          "TestLocationRoute: message arrived for entity={}\n", msg->entity_
        );
        EXPECT_EQ(msg->entity_, location::offset + msg->from_);
        msg_count++;
      }
    );

    // send long messages for entity
    using MsgType = location::EntityMsg;
    bool is_long = std::is_same<TypeParam,location::LongMsg>::value;
    auto msg = vt::makeSharedMessage<MsgType>(entity, my_node, is_long);
    vt::theMsg()->broadcastMsg<MsgType, location::routeTestHandler<TypeParam>>(msg);

    while (msg_count < nb_nodes - 1) { vt::runScheduler(); }

    debug_print(
      location, node,
      "TestLocationRoute: all messages have been arrived\n"
    );

    EXPECT_EQ(msg_count, nb_nodes - 1);
  }
}

TYPED_TEST_P(TestLocationRoute, test_entity_cache_hits) /* NOLINT */ {

  auto const nb_nodes  = vt::theContext()->getNumNodes();
  auto const my_node   = vt::theContext()->getNode();
  auto const entity    = location::default_entity;
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
  unit::location::verifyCacheConsistency<TypeParam>(
    entity, my_node, home, home, nb_rounds
  );

  // finalize
  if (my_node == home) {
    EXPECT_EQ(nb_received, (nb_nodes - 1) * nb_rounds);
  }
}

TYPED_TEST_P(TestLocationRoute, test_entity_cache_migrated_entity) /* NOLINT */{

  auto const nb_nodes  = vt::theContext()->getNumNodes();

  // cannot perform entity migration if less than 3 nodes
  if (nb_nodes > 2) {
    auto const my_node   = vt::theContext()->getNode();
    auto const entity    = location::default_entity;
    auto const home      = 0;
    auto const new_home  = 3;
    auto const nb_rounds = 5;
    auto nb_received     = 0;

    // register entity
    if (my_node == home) {
      vt::theLocMan()->virtual_loc->registerEntity(entity, my_node);
    }
    vt::theCollective()->barrier();

    if (my_node == home) {
      // migrate entity: unregister it but keep its id in cache
      vt::theLocMan()->virtual_loc->entityMigrated(entity, new_home);
      EXPECT_TRUE(location::isCached(entity));
    } else if (my_node == new_home) {
      // receive migrated entity: register it and keep in cache
      vt::theLocMan()->virtual_loc->registerEntityMigrated(
        entity, my_node, [entity,&nb_received](vt::BaseMessage* in_msg) {
          debug_print(
            location, node,
            "TestLocationRoute: message arrived to me for a migrated entity={}\n",
            entity
          );
          nb_received++;
        }
      );
      EXPECT_TRUE(location::isCached(entity));
    }

    // check cache consistency for the given entity
    location::verifyCacheConsistency<TypeParam>(
      entity, my_node, home, new_home, nb_rounds
    );

    // finalize
    if (my_node == new_home) {
      auto const min_expected_ack = (nb_nodes - 2) * nb_rounds;
      EXPECT_TRUE(nb_received >= min_expected_ack);
    }
  }
}

REGISTER_TYPED_TEST_SUITE_P(
  TestLocationRoute,
  test_route_entity, test_entity_cache_hits, test_entity_cache_migrated_entity
);

using LocationMsgType = location::MsgType;

INSTANTIATE_TYPED_TEST_SUITE_P(
  test_location_message, TestLocationRoute, LocationMsgType, DEFAULT_NAME_GEN
);

}}} // end namespace vt::tests::unit
