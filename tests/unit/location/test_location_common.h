/*
//@HEADER
// ************************************************************************
//
//                    test_location_common.h
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

#if !defined INCLUDED_TEST_LOCATION_COMMON_H
#define INCLUDED_TEST_LOCATION_COMMON_H

#include "data_message.h"
#include "vt/transport.h"
#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit { namespace location {

// constants used in test cases
int const arbitrary_entity = 10;
int const magic_number     = 29;
int const invalid_entity   = -1;

struct EntityMsg : vt::Message {

  EntityMsg(int in_entity, vt::NodeType in_home, bool in_large = false)
    : entity_(in_entity),
      home_(in_home),
      is_large_(in_large)
  {}

  int entity_ = invalid_entity;
  vt::NodeType home_ = vt::uninitialized_destination;
  bool is_large_ = false;
};

struct ShortMsg : vt::LocationRoutedMsg<int, vt::ShortMessage> {

  ShortMsg(int in_entity, vt::NodeType in_from)
    : entity_(in_entity),
      from_(in_from)
  {}

  vt::NodeType from_ = vt::uninitialized_destination;
  int entity_ = invalid_entity;
};

struct LongMsg : vt::LocationRoutedMsg<int, vt::Message> {

  LongMsg(int in_entity, vt::NodeType in_from)
    : entity_(in_entity),
      from_(in_from)
  {}

  vt::NodeType from_ = vt::uninitialized_destination;
  int entity_ = invalid_entity;
  double additional_data_[50];
};

template <typename MsgT>
void routeTestHandler(EntityMsg* msg) {

  auto const my_node = vt::theContext()->getNode();
  auto test_msg = vt::makeMessage<MsgT>(magic_number + my_node, my_node);
  auto const& test_msg_size = sizeof(*test_msg);

  // check message size
  bool correct_size = (
    msg->is_large_ ?
      (vt::location::small_msg_max_size < test_msg_size) :
      (vt::location::small_msg_max_size >= test_msg_size)
  );

  EXPECT_TRUE(correct_size);

  debug_print(
    location, node,
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
};

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

  vt::theCollective()->barrier();
  // the protocol is defined as eager for short and unserialized messages
  bool const is_eager = std::is_same<MsgT,ShortMsg>::value;

  for (int iter = 0; iter < nb_rounds; ++iter) {
    if (my_node not_eq home) {
      // route entity message
      auto msg = vt::makeMessage<MsgT>(entity, my_node);
      vt::theLocMan()->virtual_loc->routeMsg<MsgT>(entity, home, msg);

      // check for cache updates
      bool is_entity_cached = isCached(entity);

      debug_print(
        location, node,
        "vertifyCacheConsistency: iter={}, entityID={}, home={}, bytes={}, "
        "in cache={}\n",
        iter, msg->entity_, msg->from_, sizeof(*msg), is_entity_cached
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
        // Thus the cache is not updated in this case.
        EXPECT_FALSE(is_entity_cached);
      }
    } else {
      // The entity should be registered in the cache of the home node,
      // regardless of the protocol (eager or not)
      EXPECT_TRUE(isCached(entity));
    }
    // wait for the termination of all ranks
    vt::theCollective()->barrier();
  }
}

// message types used for type-parameterized tests
using MsgType = testing::Types<ShortMsg, LongMsg>;

}}}} // namespace vt::tests::unit::location

#endif /*INCLUDED_TEST_LOCATION_COMMON_H*/
