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
	
	static vt::NodeType me;
	static vt::NodeType root;
	static vt::NodeType all;
	static std::map<vt::EpochType,Metadata> data;
	
	
	// basic message
	struct BasicMsg : vt::Message {
		
		BasicMsg() = default;
		BasicMsg(vt::NodeType src, vt::NodeType dst, int ttl, vt::EpochType epoch) :
			src_(src),
			dst_(dst),
			ttl_(ttl-1),
			epoch_(epoch) {}
		BasicMsg(vt::NodeType src, vt::NodeType dst, int ttl) : BasicMsg(src,dst,ttl,vt::no_epoch){}
		BasicMsg(vt::NodeType src, vt::NodeType dst) : BasicMsg(src,dst,4,vt::no_epoch){}
		
		//
		int ttl_ = 3;
		vt::NodeType src_ = vt::uninitialized_destination;
		vt::NodeType dst_ = vt::uninitialized_destination;
		vt::EpochType epoch_ = vt::no_epoch;
	};

	// -----------------------------------------
	// - control messages for channel counting -
	// -----------------------------------------
	struct PingMsg : vt::Message {
		
		PingMsg() = default;
		PingMsg(vt::NodeType src, vt::NodeType dst, int nb, vt::EpochType epoch) :
			src_(src),
			dst_(dst),
			nb_out_(nb),
			epoch_(epoch) {}
		PingMsg(vt::NodeType src, vt::NodeType dst, int nb) : PingMsg(src,dst,nb,vt::no_epoch){}
		PingMsg(vt::NodeType src, vt::NodeType dst) : PingMsg(src,dst,0,vt::no_epoch){}
		
		int nb_out_ = 0;
		vt::NodeType src_ = vt::uninitialized_destination;
		vt::NodeType dst_ = vt::uninitialized_destination;
		vt::EpochType epoch_ = vt::no_epoch;
	};
	
	struct EchoMsg : vt::Message {
		
		EchoMsg() = default;
		EchoMsg(vt::NodeType src, vt::NodeType dst, int nb, vt::EpochType epoch) :
			src_(src),
			dst_(dst),
			nb_in_(nb),
			epoch_(epoch) {}
		EchoMsg(vt::NodeType src, vt::NodeType dst, int nb) : EchoMsg(src,dst,nb,vt::no_epoch){}
		EchoMsg(vt::NodeType src, vt::NodeType dst) : EchoMsg(src,dst,0,vt::no_epoch){}
		
		int nb_in_ = 0;
		vt::NodeType src_ = vt::uninitialized_destination;
		vt::NodeType dst_ = vt::uninitialized_destination;
		vt::EpochType epoch_ = vt::no_epoch;
	};
	
	// ---------------------------------
	// meta-data per rank and per epoch
	// for channel counting algorithm
	// ---------------------------------
	struct Metadata {
		
		int degree_;
		int activator_;
		std::vector<int> in_;       // Cij^-
		std::vector<int> out_;      // Cij^+
		std::vector<int> ack_;
		
		Metadata() : degree_(0),activator_(0){
			out_.resize(all,0);
			in_.resize(all,0);
			ack_.resize(all,0);
		}
		Metadata(int act) : Metadata(){ activator_ = act; }
	};
	
	// shortcuts for message sending
	static void sendBasic(vt::NodeType dst, vt::EpochType ep){
		vtAssertExpr(dst not_eq me);
		auto msg = makeSharedMessage<BasicMsg>(me,dst,1,ep);
		if(ep not_eq vt::no_epoch){
			vt::envelopeSetEpoch(msg->env,ep);
		}
		vt::theMsg()->sendMsg<BasicMsg,basicHandler>(dst,msg);
		// increment outgoing message counter
		data[ep].out_[dst]++;
	}
	
	static void routeBasic(vt::NodeType dst, vt::NodeType ttl, vt::EpochType ep){
		vtAssertExpr(dst not_eq me);
		auto msg = makeSharedMessage<BasicMsg>(me,dst,ttl,ep);
		if(ep not_eq vt::no_epoch){
			vt::envelopeSetEpoch(msg->env,ep);
		}
		vt::theMsg()->sendMsg<BasicMsg,routedHandler>(dst,msg);
		// increment outgoing message counter
		data[ep].out_[dst]++;
	}
	
	static void sendPing(vt::NodeType dst, vt::NodeType count, vt::EpochType ep){
		vtAssertExpr(dst not_eq me);
		auto msg = makeSharedMessage<PingMsg>(me,dst,count,ep);
		if(ep not_eq vt::no_epoch){
			vt::envelopeSetEpoch(msg->env,ep);
		}
		vt::theMsg()->sendMsg<PingMsg,pingHandler>(dst,msg);
	}
	
	static void sendEcho(vt::NodeType dst, vt::NodeType count, vt::EpochType ep){
		vtAssertExpr(dst not_eq me);
		auto msg = makeSharedMessage<EchoMsg>(me,dst,count,ep);
		if(ep not_eq vt::no_epoch){
			vt::envelopeSetEpoch(msg->env,ep);
		}
		vt::theMsg()->sendMsg<EchoMsg,echoHandler>(dst,msg);
	}
	
	// propagate check on current subtree
	static void propagate(vt::EpochType ep){
		for(int i=0; i < all; i++){
			if(i not_eq me){
				// confirmation missing
				if(data[ep].out_[i] not_eq data[ep].ack_[i]){
					// check subtree
					sendPing(i,data[ep].out_[i],ep);
					// more echoes outstanding
					data[ep].degree_++;
				}
			}
		}
	}
	
	// check local termination
	// check whether Cij^+ == Cij^-
	static bool hasFinished(vt::EpochType ep){
		for(int i=0; i < all; ++i){
			if(i not_eq me){
				if(data[ep].out_[i] not_eq data[ep].ack_[i]){
					return false;
				}
			}
		}
		return true;
	}
	
	// on receipt of a basic message
	static void basicHandler(BasicMsg* msg){
		vtAssertExpr(me == msg->dst_);
		auto const& src = msg->src_;
		vtAssertExpr(src < all);
		auto const& ep = msg->epoch_;
		// when Pj send to Pi, increment out_[i]
		data[ep].in_[src]++;
	}
	
	static void routedHandler(BasicMsg* msg){
		vtAssertExpr(me not_eq root);
		basicHandler(msg);
		
		if (msg->ttl_ > 0){
			int const nb_rounds = static_cast<int>(drand48()*5);
			for(int k=0; k < nb_rounds; ++k){
				int dst = (me+1 > all-1 ? 1: me+1);
				routeBasic(dst,msg->ttl_,msg->epoch_);
			}
		}
	}
	
	// on receipt of an echo message echo<m> from Pi
	static void echoHandler(EchoMsg* msg){
		vtAssertExpr(me == msg->dst_);
		int const src = msg->src_;
		vtAssertExpr(src < all);
		auto const& ep = msg->epoch_;
		
		data[ep].ack_[src] = msg->nb_in_;
		// decrease missing echoes counter
		data[ep].degree_--;
		// last echo checks whether all subtrees are quiet
		if(data[ep].degree_ == 0){
			propagate(ep);
		}
		// all echoes arrived, everything quiet
		if(data[ep].degree_ == 0){
			int const activator = data[ep].activator_;
			if(me not_eq activator){
				sendEcho(activator,data[ep].in_[activator],ep);
			}
		}
	}
	
	// on receipt of a control message test<m> from Pi
	static void pingHandler(PingMsg* msg){
		vtAssertExpr(me == msg->dst_);
		auto const& src = msg->src_;
		auto const& ep = msg->epoch_;
		
		// if already engaged or subtree is quiet
		if(data[ep].degree_ > 0 or hasFinished(ep)){
			sendEcho(src,data[ep].in_[src],ep);
		}
		else {
			data[ep].activator_ = src;
			propagate(ep);
		}
	}
	
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
};

/*static*/ vt::NodeType TestTermAction::me;
/*static*/ vt::NodeType TestTermAction::root;
/*static*/ vt::NodeType TestTermAction::all;
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
			sendPing(i,data[vt::no_epoch].out_[i],vt::no_epoch);
		}
	
		// check global termination
		theTerm()->addAction([=]{
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
			sendPing(i,data[vt::no_epoch].out_[i],vt::no_epoch);
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
				sendPing(i,data[ep[i]].out_[i],ep[i]);
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
