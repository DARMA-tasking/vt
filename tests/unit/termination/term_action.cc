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
		using TestMsg = TestStaticBytesNormalMsg<4>;

		// count the number of msgs received
		static NodeType root;
		static int ack_count;
		static int sen_count;
		static int rcv_count;

		virtual void SetUp(){
			TestParallelHarness::SetUp();

			sen_count = 5;
			rcv_count = 0;
			ack_count = 0;
			root = 0;
		}

		virtual void TearDown(){
			TestParallelHarness::TearDown();
		}

		// handlers
		static void rcv_handler(TestMsg* msg){
			auto ack = makeSharedMessage<TestMsg>();
			theMsg()->sendMsg<TestMsg,ack_handler>(root,ack);

			rcv_count++;
		}

		static void ack_handler(TestMsg* msg){
			ack_count++;
		}
	};

/*static*/ NodeType TestTermAction::root;
/*static*/ int TestTermAction::ack_count;
/*static*/ int TestTermAction::sen_count;
/*static*/ int TestTermAction::rcv_count;


TEST_F(TestTermAction, test_termination_action_global_broadcast)
{
	auto const& my_node = theContext()->getNode();
	auto const& num_nodes = theContext()->getNumNodes();

	// case 1: node 0 send a msg to a group of nodes
	// check that all messages have been received before
	// any action stored in 'addAction' is actually triggered.
	if(num_nodes > 1){
		if(my_node == root){
			for(int i=0; i < sen_count; ++i){
				auto msg = makeSharedMessage<TestMsg>();
				theMsg()->broadcastMsg<TestMsg,rcv_handler>(msg);
			}
		}

		theTerm()->addAction([=]{
			if(my_node == root){
				#if DEBUG_TEST_HARNESS_PRINT
					fmt::print("root: {} ack(s) received.\n", ack_count);
				#endif
					
				/*auto const& status = theTerm()->testEpochFinished(term::any_epoch_sentinel,nullptr);
				if(status == term::TermStatusEnum::Finished){
					fmt::print("root: yes global termination.");
				}
				EXPECT_EQ(status, term::TermStatusEnum::Finished);*/

				// 'handler_count' is incremented if only if all messages sent by node 0
				// has been received by each node of the group.
				int const expected = sen_count*(num_nodes-1);
				EXPECT_EQ(ack_count,expected);
			}
			else {
				EXPECT_EQ(rcv_count,sen_count);
			}
		});
	}
}

TEST_F(TestTermAction, test_termination_action_epoch_broadcast)
{
	auto const& my_node = theContext()->getNode();
	auto const& num_nodes = theContext()->getNumNodes();

	// create a new rooted epoch
	//auto const my_epoch = theTerm()->makeEpochRooted(true);
	auto const my_epoch = theTerm()->makeEpochCollective();

	// case 1: node 0 send a msg to a group of nodes
	// check that all messages have been received before
	// any action stored in 'addAction' is actually triggered.
	if(num_nodes > 1){
		if (my_node == root){
			for(int i=0; i < sen_count; ++i){
				auto msg = makeSharedMessage<TestMsg>();
				vt::envelopeSetEpoch(msg->env, my_epoch);
				theMsg()->broadcastMsg<TestMsg,rcv_handler>(msg);
			}
		}

		theTerm()->addActionEpoch(my_epoch,[=]{
			if(my_node == root){
				#if DEBUG_TEST_HARNESS_PRINT
					fmt::print("root: terminated");
				#endif
				int const expected = sen_count*(num_nodes-1);
				EXPECT_EQ(ack_count,expected);
			}
			else {
				EXPECT_EQ(rcv_count,sen_count);
			}
		});
		theTerm()->finishedEpoch(my_epoch);
	}
}



}}} // end namespace vt::tests::unit