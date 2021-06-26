/*
//@HEADER
// *****************************************************************************
//
//                            test_location_common.h
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

#if !defined INCLUDED_UNIT_LOCATION_TEST_LOCATION_COMMON_H
#define INCLUDED_UNIT_LOCATION_TEST_LOCATION_COMMON_H

#include "data_message.h"
#include "test_parallel_harness.h"
#include "vt/topos/location/manager.h"

namespace vt { namespace tests { namespace unit { namespace location {

// constants used in test cases
int const offset         = 29;
int const default_entity = 10;
int const invalid_entity = -1;

struct EntityMsg : vt::Message {

  EntityMsg(int in_entity, vt::NodeType in_home, bool in_large = false)
    : entity_  (in_entity),
      home_    (in_home),
      is_large_(in_large)
  {}

  int entity_ = invalid_entity;
  vt::NodeType home_ = vt::uninitialized_destination;
  bool is_large_ = false;
};

struct ShortMsg : vt::LocationRoutedMsg<int, vt::ShortMessage> {

  ShortMsg(int in_entity, vt::NodeType in_from)
    : from_(in_from),
      entity_(in_entity)
  {}

  vt::NodeType from_ = vt::uninitialized_destination;
  int entity_ = invalid_entity;
};

struct LongMsg : vt::LocationRoutedMsg<int, vt::Message> {

  LongMsg(int in_entity, vt::NodeType in_from)
    : from_(in_from),
      entity_(in_entity),
      data_ ()
  {
    std::memset(data_, 0, vt::location::small_msg_max_size);
  }

  vt::NodeType from_ = vt::uninitialized_destination;
  int entity_ = invalid_entity;
  char data_[vt::location::small_msg_max_size] = "";
};

struct SerialMsg : ShortMsg {
  SerialMsg(int in_entity, vt::NodeType in_from)
    : ShortMsg(in_entity, in_from)
  {
    setSerialize(true);
  }
};

template <typename MsgT>
void routeTestHandler(EntityMsg* msg) {

  auto const my_node = vt::theContext()->getNode();
  auto test_msg = vt::makeMessage<MsgT>(offset + my_node, my_node);
  auto const& test_msg_size = sizeof(*test_msg);

  // check message size
  bool correct_size = (
    msg->is_large_ ?
      (vt::location::small_msg_max_size < test_msg_size) :
      (vt::location::small_msg_max_size >= test_msg_size)
  );

  EXPECT_TRUE(correct_size);

  vt_debug_print(
    normal, location,
    "routeTestHandler: entity={}, home={}, bytes={}\n",
    msg->entity_, msg->home_, test_msg_size
  );
  // route message
  vt::theLocMan()->virtual_loc->routeMsg<MsgT>(
    msg->entity_, msg->home_, test_msg
  );
}

// check if the given entity is in the node cache
bool isCached(int const entity) {
  return vt::theLocMan()->virtual_loc->isCached(entity);
}

// check if the given entity should be stored in cache or not
// depending on the situation:
// - eager or non eager protocol
// - current iteration
// - (previous) home node or not
template <typename MsgT>
void verifyCacheConsistency(
  int const entity, vt::NodeType const my_node,
  vt::NodeType const home, vt::NodeType const new_home, int const nb_rounds
) {

  for (int iter = 0; iter < nb_rounds; ++iter) {
    // create an entity message to route
    auto msg = vt::makeMessage<MsgT>(entity, my_node);
    // check if should be serialized or not
    bool serialize = msg->getSerialize();

    // perform the checks only after all entity messages have been
    // correctly delivered
    runInEpochCollective([&]{
      if (my_node not_eq home) {
        // route entity message
        vt::theLocMan()->virtual_loc->routeMsg<MsgT>(entity, home, msg, serialize);
      }
    });

    if (my_node not_eq home) {
      // check the routing protocol to be used by the manager.
      bool is_eager = theLocMan()->virtual_loc->useEagerProtocol(msg);

      // check for cache updates
      bool is_entity_cached = isCached(entity);

      vt_debug_print(
        normal, location,
        "verifyCacheConsistency: iter={}, entityID={}, home={}, bytes={}, "
        "in cache={}, serialize={}\n",
        iter, entity, msg->from_, sizeof(*msg), is_entity_cached, serialize
      );

      if (not is_eager) {
        // On non eager case: the location is first explicitly resolved
        // and then the message is routed and the cache updated.
        // Hence, the entity is not yet registered in cache after the
        // first send, but will be for next ones.
        EXPECT_TRUE(iter < 1 or is_entity_cached);
      } else if(my_node not_eq new_home) {
        // On eager case: the message is directly routed to the
        // implicitly resolved location.
        // Eager update is sent, updating the cache
        EXPECT_TRUE(is_entity_cached);
      }
    } else /* my_node == home */ {
      // The entity should be registered in the cache of the home node,
      // regardless of the protocol (eager or not)
      EXPECT_TRUE(isCached(entity));
    }
  }
}

// message types used for type-parameterized tests
using MsgType = testing::Types<ShortMsg, LongMsg, SerialMsg>;

}}}} // namespace vt::tests::unit::location

#endif /*INCLUDED_UNIT_LOCATION_TEST_LOCATION_COMMON_H*/
