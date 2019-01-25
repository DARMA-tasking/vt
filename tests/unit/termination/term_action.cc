/*
//@HEADER
// ************************************************************************
//
//                          term_term.cc
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
#include <random>

#include "test_parallel_harness.h"
#include "data_message.h"
#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

// basic message
struct BasicMsg : vt::Message {
	
	BasicMsg() = default;
	BasicMsg(int p_src, int p_dst, int p_ttl, vt::EpochType p_epoch) :
		src(p_src),
		dst(p_dst),
		ttl(p_ttl-1),
		epoch(p_epoch) {}
	BasicMsg(int p_src, int p_dst, int p_ttl) : BasicMsg(p_src,p_dst,p_ttl,vt::no_epoch){}
	BasicMsg(int p_src, int p_dst) : BasicMsg(p_src,p_dst,4,vt::no_epoch){}
	
	//
	int ttl = 3;
	int src = -1;
	int dst = -1;
	vt::EpochType epoch = vt::no_epoch;
};

// control messages
struct PingMsg : vt::Message {
	
	PingMsg() = default;
	PingMsg(int p_src, int p_dst, int p_nb, vt::EpochType p_epoch) :
		src(p_src),
		dst(p_dst),
		nb_out(p_nb),
		epoch(p_epoch) {}
	PingMsg(int p_src, int p_dst, int p_nb) : PingMsg(p_src,p_dst,p_nb,vt::no_epoch){}
	PingMsg(int p_src, int p_dst) : PingMsg(p_src,p_dst,0,vt::no_epoch){}
	
	int nb_out = 0;
	int src = -1;
	int dst = -1;
	vt::EpochType epoch = vt::no_epoch;
};

struct EchoMsg : vt::Message {
	
	EchoMsg() = default;
	EchoMsg(int p_src, int p_dst, int p_nb, vt::EpochType p_epoch) :
		src(p_src),
		dst(p_dst),
		nb_in(p_nb),
		epoch(p_epoch) {}
	EchoMsg(int p_src, int p_dst, int p_nb) : EchoMsg(p_src,p_dst,p_nb,vt::no_epoch){}
	EchoMsg(int p_src, int p_dst) : EchoMsg(p_src,p_dst,0,vt::no_epoch){}
	
	int nb_in = 0;
	int src = -1;
	int dst = -1;
	vt::EpochType epoch = vt::no_epoch;
};


struct TestTermAction : vt::tests::unit::TestParallelHarness {
	
	// message counters for each MPI rank
	static std::vector<int> in;       // Cij^-
	static std::vector<int> out;      // Cij^+
	static std::vector<int> ack;      // messages received by Pi acknowledged by Pj
	static int degree;
	static int activator;
	static int me;
	static int root;
	static int all;
	static bool done;
	
	virtual void SetUp(){
		vt::tests::unit::TestParallelHarness::SetUp();
		
		// set ranks
		root = 0;
		me  = vt::theContext()->getNode();
		all = vt::theContext()->getNumNodes();
		vtAssert(all > 1, "There should be at least two nodes");
		
		// set or allocate counters
		degree = 0;
		activator = root;
		out.resize(all,0);
		in.resize(all,0);
		ack.resize(all,0);
	}
	
	virtual void TearDown(){
		vt::tests::unit::TestParallelHarness::TearDown();
	}
	
	// shortcuts for message sending
	static void sendBasic(int dst){
		vtAssertExpr(dst not_eq me);
		auto msg = makeSharedMessage<BasicMsg>(me,dst);
		vt::theMsg()->sendMsg<BasicMsg,basicHandler>(dst,msg);
		// increment out
		out[dst]++;
	}
	
	static void routeBasic(int dst,int ttl){
		vtAssertExpr(dst not_eq me);
		auto msg = makeSharedMessage<BasicMsg>(me,dst,ttl);
		vt::theMsg()->sendMsg<BasicMsg,routedHandler>(dst,msg);
		// increment out
		out[dst]++;
	}
	
	static void sendPing(int dst, int count){
		vtAssertExpr(dst not_eq me);
		auto msg = makeSharedMessage<PingMsg>(me,dst,count);
		vt::theMsg()->sendMsg<PingMsg,pingHandler>(dst,msg);
	}
	
	static void sendEcho(int dst, int count){
		vtAssertExpr(dst not_eq me);
		auto msg = makeSharedMessage<EchoMsg>(me,dst,count);
		vt::theMsg()->sendMsg<EchoMsg,echoHandler>(dst,msg);
	}
	
	// propagate check on current subtree
	static void propagate(){
		for(int i=0; i < all; i++){
			if(i not_eq me){
				// confirmation missing
				if(out[i] not_eq ack[i]){
					// check subtree
					sendPing(i,out[i]);
					// more echoes outstanding
					degree++;
				}
			}
		}
	}
	
	// check local termination
	// check whether Cij^+ == Cij^-
	static bool hasFinished(){
		for(int i=0; i < all; ++i){
			if(i not_eq me){
				if(out[i] not_eq ack[i]){
					return false;
				}
			}
		}
		return true;
	}
	
	// on receipt of a basic message
	static void basicHandler(BasicMsg* msg){
		vtAssertExpr(me == msg->dst);
		int const& src = msg->src;
		vtAssertExpr(src < all);
		// when Pj send to Pi, increment out[i]
		in[src]++;
	}
	
	static void routedHandler(BasicMsg* msg){
		vtAssertExpr(me not_eq root);
		basicHandler(msg);
		
		if (msg->ttl > 0){
			int const nb_rounds = static_cast<int>(drand48()*5);
			for(int k=0; k < nb_rounds; ++k){
				int dst = (me+1 > all-1 ? 1: me+1);
				routeBasic(dst,msg->ttl);
			}
		}
	}
	
	// on receipt of an echo message echo<m> from Pi
	static void echoHandler(EchoMsg* msg){
		vtAssertExpr(me == msg->dst);
		auto const& src = msg->src;
		vtAssertExpr(src < all);
		
		ack[src] = msg->nb_in;
		// decrease missing echoes counter
		degree--;
		// last echo checks whether all subtrees are quiet
		if(degree == 0){
			propagate();
		}
		// all echoes arrived, everything quiet
		if(degree == 0){
			if(me not_eq activator){
				sendEcho(activator,in[activator]);
			}
		}
	}
	
	// on receipt of a control message test<m> from Pi
	static void pingHandler(PingMsg* msg){
		vtAssertExpr(me == msg->dst);
		auto const& src = msg->src;
		
		// check local termination
		bool const finished = hasFinished();
		
		// if already engaged or subtree is quiet
		if(degree > 0 or finished){
			sendEcho(src,in[src]);
		}
		else {
			activator = src;
			propagate();
		}
	}
};

/*static*/ std::vector<int> TestTermAction::in;
/*static*/ std::vector<int> TestTermAction::out;
/*static*/ std::vector<int> TestTermAction::ack;
/*static*/ int TestTermAction::degree;
/*static*/ int TestTermAction::activator;
/*static*/ int TestTermAction::me;
/*static*/ int TestTermAction::root;
/*static*/ int TestTermAction::all;

TEST_F(TestTermAction, test_term_detect_broadcast)
{
	if(me == root){
		// start computation
		for(int i=1; i < all; ++i){
			sendBasic(i);
		}
	
		// retrieve counters from other nodes
		for(int i=1; i < all; ++i){
			sendPing(i,out[i]);
		}
	
		// check counters when everything is done
		theTerm()->addAction([=]{
			// check local termination
			EXPECT_TRUE(hasFinished());
		});
	}
}

TEST_F(TestTermAction, test_term_detect_routed)
{
	if(me == root){
	
		std::random_device device;
		std::mt19937 engine(device());
		
		std::uniform_int_distribution<int> dist_ttl(1,10);
		std::uniform_int_distribution<int> dist_dest(1,all-1);
		std::uniform_int_distribution<int> dist_round(1,10);
		
		int const rounds = dist_round(engine);
		
		// start computation
		for(int k=0; k < rounds; ++k){
			int dst = me + dist_dest(engine);
			int ttl = dist_ttl(engine);
			routeBasic(dst,ttl);
		}
		
		// retrieve counters from other nodes
		for(int i=1; i < all; ++i){
			sendPing(i,out[i]);
		}
		
		// check counters when everything is done
		theTerm()->addAction([=]{
			// check local termination
			EXPECT_TRUE(hasFinished());
		});
	}
}

}}} // end namespace vt::tests::unit
