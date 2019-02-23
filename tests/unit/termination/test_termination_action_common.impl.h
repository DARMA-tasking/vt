/*
//@HEADER
// ************************************************************************
//
//                 test_termination_action_common.impl.h
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

#if !defined INCLUDED_TERMINATION_ACTION_COMMON_IMPL_H
#define INCLUDED_TERMINATION_ACTION_COMMON_IMPL_H

namespace vt { namespace tests { namespace unit { namespace action {

// epoch sequence creation [rooted,collect]
std::vector<vt::EpochType> newEpochSeq(int nb, bool rooted, bool useDS) {

  vtAssertExpr(nb > 0);
  std::vector<vt::EpochType> seq(nb);

  if (rooted) {
    vtAssertExpr(channel::me == channel::root);
    // create rooted epoch sequence
    seq[0] = vt::theTerm()->makeEpochRooted(useDS);
    vtAssertExpr(channel::root == epoch::EpochManip::node(seq[0]));
    vtAssertExpr(epoch::EpochManip::isRooted(seq[0]));

    for (int i=1; i < nb; ++i){
      seq[i] = epoch::EpochManip::next(seq[i-1]);
      vtAssertExpr(epoch::EpochManip::isRooted(seq[i]));
    }
  } else /*collective*/ {
    for (auto&& ep : seq) {
      ep = vt::theTerm()->makeEpochCollective();
    }
  }
  return seq;
}

// fictive distributed computation of a given epoch
void compute(vt::EpochType const& ep) {

  std::random_device device;
  std::mt19937 engine(device());
  std::uniform_int_distribution<int> dist_ttl(1,10);
  std::uniform_int_distribution<int> dist_dest(1,channel::all-1);
  std::uniform_int_distribution<int> dist_round(1,10);

  #if DEBUG_TERM_ACTION
    if(ep not_eq vt::no_epoch){
      fmt::print(
        "rank:{}, epoch:{:x}, is_rooted ? {}\n",
        channel::me, ep, epoch::EpochManip::isRooted(ep)
      );
    }
  #endif

  // process computation for current epoch
  int rounds = dist_round(engine);
  if (channel::all > 2) {
    for (int k = 0; k < rounds; ++k) {
      int dst = channel::me + dist_dest(engine);
      int ttl = dist_ttl(engine);
      channel::routeBasic(dst,ttl,ep);
    }
  } else {
    channel::broadcast<channel::basicHandler>(1,ep);
  }
}

/*
 * at the end of a given epoch:
 * - assign action to be processed
 * - trigger termination detection
 * - finish epoch
 */
void finalize(vt::EpochType const ep, Order const& order) {

  auto finish = [&](vt::EpochType const& ep) {
    if (ep not_eq vt::no_epoch) {
      vt::theTerm()->finishedEpoch(ep);
    }
  };

  if (ep == vt::no_epoch) {
    if (channel::me == channel::root) {
      channel::trigger(ep);
      verify(ep,order);
    }
  } else if (epoch::EpochManip::isRooted(ep)) /* real rooted epoch */ {
    if (channel::me == channel::root) {

      switch(order){
        case Order::after: {
          channel::trigger(ep);
          finish(ep);
          verify(ep,order);
          break;
        }
        case Order::misc: {
          channel::trigger(ep);
          verify(ep,order);
          finish(ep);
          break;
        }
        default: {
          verify(ep, order);
          channel::trigger(ep);
          finish(ep);
          break;
        }
      }
    }
  } else /* real collective epoch */ {
    finish(ep);
    if (channel::me == channel::root) { verify(ep,order); }
  }

#if DEBUG_TERM_ACTION
  fmt::print("rank:{}, epoch:{:x} completed\n", channel::me, ep);
#endif
}

void verify(vt::EpochType const& ep, Order const& order){
#if DEBUG_TERM_ACTION
  auto hasEnded = [](vt::EpochType const& ep, Order const& order){
    print("within", ep, order);
    EXPECT_TRUE(channel::hasEnded(ep));
    print("leaving", ep, order);
  };

  print("entering", ep, order);
  if(ep == vt::no_epoch){
    vt::theTerm()->addAction([&]{ hasEnded(ep,order); });
  } else {
    vt::theTerm()->addActionEpoch(ep,[&]{ hasEnded(ep,order); });
  }
#else
  if (ep == vt::no_epoch) {
    vt::theTerm()->addAction([&]{ EXPECT_TRUE(channel::hasEnded(ep)); });
  } else {
    vt::theTerm()->addActionEpoch(ep,[&]{ EXPECT_TRUE(channel::hasEnded(ep)); });
  }
#endif
}


void print(std::string step, vt::EpochType const& ep, Order const& order) {
#if DEBUG_TERM_ACTION
  std::string text = "";

  switch (order) {
    case Order::after: text = "order::after"; break;
    case Order::misc: text = "order::misc"; break;
    default: text = "order::before"; break;
  }

  if (ep not_eq vt::no_epoch) {
    fmt::print(
      "rank:{}: epoch:{:x}, {}, {} action, is_rooted ? {}\n",
      channel::me, ep, text, step, epoch::EpochManip::isRooted(ep)
    );
  } else /* no_epoch */ {
    fmt::print("rank:{}: no_epoch, {}, {} action\n", channel::me, text, step);
  }
  fflush(stdout);
#endif
}

}}}} // end namespace vt::tests::unit::action

#endif /*INCLUDED_TERMINATION_ACTION_COMMON_IMPL_H*/