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

struct TestTermAction : vt::tests::unit::TestParallelHarness {

	struct Metadata; // forward-declaration for 'data'
	
	static int me;
	static int root;
	static int all;
	static std::map<vt::EpochType,Metadata> data;
	
	virtual void SetUp(){
		vt::tests::unit::TestParallelHarness::SetUp();
		
		// set ranks
		root = 0;
		me  = vt::theContext()->getNode();
		all = vt::theContext()->getNumNodes();
		vtAssert(all > 1, "There should be at least two nodes");
	}
	
	virtual void TearDown(){
		vt::tests::unit::TestParallelHarness::TearDown();
	}
	
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
	
	struct Metadata {
		int degree;
		int activator;
		// counters
		std::vector<int> in;       // Cij^-
		std::vector<int> out;      // Cij^+
		std::vector<int> ack;
		
		Metadata(){
			degree = 0;
			activator = 0;
			out.resize(all,0);
			in.resize(all,0);
			ack.resize(all,0);
		}
		Metadata(int act) : Metadata(){ activator = act; }
	};
	
	// message counters for each MPI rank
	/*static std::vector<int> in;       // Cij^-
	static std::vector<int> out;      // Cij^+
	static std::vector<int> ack;
	static int degree;
	static int activator;
	static int me;
	static int root;
	static int all;
	static bool done;*/
	
	
	
	// shortcuts for message sending
	static void sendBasic(int dst, vt::EpochType ep){
		vtAssertExpr(dst not_eq me);
		auto msg = makeSharedMessage<BasicMsg>(me,dst,1,ep);
		if(ep not_eq vt::no_epoch){
			vt::envelopeSetEpoch(msg->env,ep);
		}
		vt::theMsg()->sendMsg<BasicMsg,basicHandler>(dst,msg);
		// increment out
		data[ep].out[dst]++;
	}
	
	static void routeBasic(int dst,int ttl, vt::EpochType ep){
		vtAssertExpr(dst not_eq me);
		auto msg = makeSharedMessage<BasicMsg>(me,dst,ttl,ep);
		if(ep not_eq vt::no_epoch){
			vt::envelopeSetEpoch(msg->env,ep);
		}
		vt::theMsg()->sendMsg<BasicMsg,routedHandler>(dst,msg);
		// increment out
		data[ep].out[dst]++;
	}
	
	static void sendPing(int dst, int count, vt::EpochType ep){
		vtAssertExpr(dst not_eq me);
		auto msg = makeSharedMessage<PingMsg>(me,dst,count,ep);
		if(ep not_eq vt::no_epoch){
			vt::envelopeSetEpoch(msg->env,ep);
		}
		vt::theMsg()->sendMsg<PingMsg,pingHandler>(dst,msg);
	}
	
	static void sendEcho(int dst, int count, vt::EpochType ep){
		vtAssertExpr(dst not_eq me);
		auto msg = makeSharedMessage<EchoMsg>(me,dst,count,ep);
		if(ep not_eq vt::no_epoch){
			vt::envelopeSetEpoch(msg->env,ep);
		}
		vt::theMsg()->sendMsg<EchoMsg,echoHandler>(dst,msg);
	}
	
	// no epochs
	static void sendBasic(int dst){ sendBasic(dst,vt::no_epoch); }
	static void routeBasic(int dst,int ttl){ routeBasic(dst,ttl,vt::no_epoch); }
	static void sendPing(int dst, int count){ sendPing(dst,count,vt::no_epoch); }
	static void sendEcho(int dst, int count){ sendEcho(dst,count,vt::no_epoch); }
	
	// propagate check on current subtree
	static void propagate(vt::EpochType ep){
		for(int i=0; i < all; i++){
			if(i not_eq me){
				// confirmation missing
				if(data[ep].out[i] not_eq data[ep].ack[i]){
					// check subtree
					sendPing(i,data[ep].out[i],ep);
					// more echoes outstanding
					data[ep].degree++;
				}
			}
		}
	}
	
	// check local termination
	// check whether Cij^+ == Cij^-
	static bool hasFinished(vt::EpochType ep){
		for(int i=0; i < all; ++i){
			if(i not_eq me){
				if(data[ep].out[i] not_eq data[ep].ack[i]){
					return false;
				}
			}
		}
		return true;
	}
	
	// on receipt of a basic message
	static void basicHandler(BasicMsg* msg){
		vtAssertExpr(me == msg->dst);
		auto const& src = msg->src;
		vtAssertExpr(src < all);
		auto const& ep = msg->epoch;
		// when Pj send to Pi, increment out[i]
		data[ep].in[src]++;
	}
	
	static void routedHandler(BasicMsg* msg){
		vtAssertExpr(me not_eq root);
		basicHandler(msg);
		
		if (msg->ttl > 0){
			int const nb_rounds = static_cast<int>(drand48()*5);
			for(int k=0; k < nb_rounds; ++k){
				int dst = (me+1 > all-1 ? 1: me+1);
				routeBasic(dst,msg->ttl,msg->epoch);
			}
		}
	}
	
	// on receipt of an echo message echo<m> from Pi
	static void echoHandler(EchoMsg* msg){
		vtAssertExpr(me == msg->dst);
		int const src = msg->src;
		vtAssertExpr(src < all);
		auto const& ep = msg->epoch;
		
		data[ep].ack[src] = msg->nb_in;
		// decrease missing echoes counter
		data[ep].degree--;
		// last echo checks whether all subtrees are quiet
		if(data[ep].degree == 0){
			propagate(ep);
		}
		// all echoes arrived, everything quiet
		if(data[ep].degree == 0){
			int const activator = data[ep].activator;
			if(me not_eq activator){
				sendEcho(activator,data[ep].in[activator],ep);
			}
		}
	}
	
	// on receipt of a control message test<m> from Pi
	static void pingHandler(PingMsg* msg){
		vtAssertExpr(me == msg->dst);
		auto const& src = msg->src;
		auto const& ep = msg->epoch;
		
		// if already engaged or subtree is quiet
		if(data[ep].degree > 0 or hasFinished(ep)){
			sendEcho(src,data[ep].in[src],ep);
		}
		else {
			data[ep].activator = src;
			propagate(ep);
		}
	}
};

/*static*/ int TestTermAction::me;
/*static*/ int TestTermAction::root;
/*static*/ int TestTermAction::all;
/*static*/ std::map<vt::EpochType,TestTermAction::Metadata> TestTermAction::data;

TEST_F(TestTermAction, test_term_detect_broadcast)
{
	if(me == root){
		
		// start computation
		for(int i=1; i < all; ++i){
			sendBasic(i,vt::no_epoch);
		}
	
		// retrieve counters from other nodes
		for(int i=1; i < all; ++i){
			sendPing(i,data[vt::no_epoch].out[i],vt::no_epoch);
		}
	
		// check counters when everything is done
		theTerm()->addAction([=]{
			// check local termination
			EXPECT_TRUE(hasFinished(vt::no_epoch));
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
			routeBasic(dst,ttl,vt::no_epoch);
		}
		
		// retrieve counters from other nodes
		for(int i=1; i < all; ++i){
			sendPing(i,data[vt::no_epoch].out[i],vt::no_epoch);
		}
		
		// check counters when everything is done
		theTerm()->addAction([=]{
			// check local termination
			EXPECT_TRUE(hasFinished(vt::no_epoch));
		});
	}
}

TEST_F(TestTermAction, test_term_detect_epoch)
{
	int const nb_ep = 4;
	vt::EpochType ep[nb_ep];

	// create epochs in a collective way
	for(int i=0; i < nb_ep; ++i){
		ep[i] = vt::theTerm()->makeEpochCollective();
	}
	
	
	if(me == root){
		std::random_device device;
		std::mt19937 engine(device());
		
		std::uniform_int_distribution<int> dist_ttl(1,10);
		std::uniform_int_distribution<int> dist_dest(1,all-1);
		std::uniform_int_distribution<int> dist_round(1,10);
		
		// process for each epoch
		for(int i=0; i < nb_ep; ++i){
			int rounds = dist_round(engine);
			// start computation
			for(int k=0; k < rounds; ++k){
				int dst = me + dist_dest(engine);
				int ttl = dist_ttl(engine);
				routeBasic(dst,ttl,ep[i]);
			}
			// send signal for termination detection
			for(int i=1; i < all; ++i){
				sendPing(i,data[ep[i]].out[i],ep[i]);
			}
		}
	}
	//
	for(int i=0; i < nb_ep; ++i){
		vt::theTerm()->finishedEpoch(ep[i]);
	}
	
	if(me == root){
		// check that all messages related to a
		// given epoch have been delivered
		// nb: only consider root counters.
		for(int i=0; i < nb_ep; ++i){
			theTerm()->addActionEpoch(ep[i],[&]{
				EXPECT_TRUE(hasFinished(ep[i]));
			});
		}
	}
}

}}} // end namespace vt::tests::unit
