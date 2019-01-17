/*
//@HEADER
// ************************************************************************
//
//                          term_reset.cc
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

	using namespace vt;
	using namespace vt::tests::unit;

	struct TestTermAction : TestParallelHarness {
		using TestMsg = TestStaticBytesShortMsg<4>;

		// count the number of msgs received
		static int32_t num_ack;

		static void respond(TestMsg* msg){
			num_ack++;
		}

		static void ping(TestMsg* msg){
			auto ack = makeSharedMessage<TestMsg>();
			theMsg()->sendMsg<TestMsg,respond>(0,ack);
		}
	};

	// initialize it
/*static*/ int32_t TestTermAction::num_ack = 0;

TEST_F(TestTermAction, test_termination_action_global_simple)
{
	auto const &this_node = theContext()->getNode();
	auto const &num_nodes = theContext()->getNumNodes();

	if (num_nodes < 2){
		return;
	}

	// case 1: node 0 send a msg to a group of nodes
	// check that all messages have been received before
	// any action stored in 'addAction' is actually triggered.
	if(this_node == 0)
	{
		// Create range for the group [1,num_nodes);
		auto range = std::make_unique<::vt::group::region::Range>(1,num_nodes);

		// The non-collective group is created by a single node based on a range or
		// list of nodes. The lambda is executed once the group is created. By
		// setting the group in the envelope of the message and broadcasting the
		// message will arrive on the set of nodes included in the group
		auto id = theGroup()->newGroup(std::move(range), [](GroupType group_id)
		{
			//fmt::print("Group is created: id={:x}\n", group_id);
			auto msg = makeSharedMessage<TestMsg>();
			envelopeSetGroup(msg->env, group_id);
			theMsg()->broadcastMsg<TestMsg,ping>(msg);
		});

		// the given lambda should not be executed if there is still some
		// messages not delivered.
		theTerm()->addAction([]{
			auto const cur_node = vt::theContext()->getNode();
			auto const num_nodes = vt::theContext()->getNumNodes();
			fmt::print("rank {}: {} ack(s) received.\n", cur_node,num_ack);
			//fmt::print("{}: terminated.\n", cur_node);
			// 'handler_count' is incremented if only if all messages sent by node 0
			// has been received by each node of the group.
			EXPECT_EQ(num_ack,num_nodes-1);
		});
	}
}

TEST_F(TestTermAction, test_termination_action_global_recursive)
{

}

// todo using recursive messages
// todo using epochs


}}} // end namespace vt::tests::unit