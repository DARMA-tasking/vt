/*
//@HEADER
// ************************************************************************
//
//                    test_location_message.h
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

#if !defined INCLUDED_TEST_LOCATION_MESSAGE_H
#define INCLUDED_TEST_LOCATION_MESSAGE_H

#include "data_message.h"
#include "vt/transport.h"
#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit { namespace locat {

// constants used in test cases
int const arbitrary_entity = 10;
int const magic_number = 29;

struct EntityMsg : vt::Message {

  EntityMsg(int in_entity, vt::NodeType in_home, bool in_large = false)
    : entity_(in_entity),
      home_(in_home),
      is_large_(in_large)
  {}

  int entity_;
  vt::NodeType home_;
  bool is_large_;
};

struct MyShortTestMsg : vt::LocationRoutedMsg<int, vt::ShortMessage> {

  MyShortTestMsg(int in_data, vt::NodeType in_from)
    : data_(in_data),
      from_(in_from)
  {}

  vt::NodeType from_ = vt::uninitialized_destination;
  int data_ = 0;
};

struct MyLongTestMsg : vt::LocationRoutedMsg<int, vt::Message> {

  MyLongTestMsg(int in_data, vt::NodeType in_from)
    : data_(in_data),
      from_(in_from)
  {}

  vt::NodeType from_ = vt::uninitialized_destination;
  int data_ = 0;
  double additional_data_[50];
};

void entityTestHandler(EntityMsg* msg) {

  auto const& my_node = vt::theContext()->getNode();

  if (not msg->is_large_) {
    auto test_short_msg = vt::makeMessage<MyShortTestMsg>(magic_number + my_node, my_node);
    auto const& short_msg_size = sizeof(*test_short_msg);
    bool correct_size = vt::location::small_msg_max_size >= short_msg_size;

    EXPECT_TRUE(correct_size);

    fmt::print(
      "{}: entityTestHandler entity={} from node={} and short message of size={}\n",
      my_node, msg->entity_, msg->home_, short_msg_size
    );

    vt::theLocMan()->virtual_loc->routeMsg<MyShortTestMsg>(
      msg->entity_, msg->home_, test_short_msg
    );

  } else {
    auto test_long_msg = vt::makeMessage<MyLongTestMsg>(magic_number + my_node, my_node);
    auto const& long_msg_size = sizeof(*test_long_msg);
    bool correct_size = vt::location::small_msg_max_size < long_msg_size;

    EXPECT_TRUE(correct_size);

    fmt::print(
      "{}: entityTestHandler entity={} from node={} and long message of size={}\n",
      my_node, msg->entity_, msg->home_, long_msg_size
    );

    vt::theLocMan()->virtual_loc->routeMsg<MyLongTestMsg>(
      msg->entity_, msg->home_, test_long_msg
    );
  }
}

}}}} // namespace vt::tests::unit::locat

#endif /*INCLUDED_TEST_LOCATION_MESSAGE_H*/
