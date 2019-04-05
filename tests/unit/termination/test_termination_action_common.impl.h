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
std::vector<vt::EpochType> generateEpochs(int nb, bool rooted, bool useDS) {
  vtAssert(nb > 0, "Invalid epoch sequence size");
  // the epoch sequence to be generated
  std::vector<vt::EpochType> sequence(nb);

  if (rooted) {
    vtAssert(channel::node == channel::root, "Node should be root");
    // create rooted epoch sequence
    sequence[0] = vt::theTerm()->makeEpochRooted(useDS);
    vtAssert(channel::root == epoch_manip::node(sequence[0]), "Should be root");
    vtAssert(epoch_manip::isRooted(sequence[0]), "First epoch should be rooted");

    for (int i = 1; i < nb; ++i){
      sequence[i] = epoch_manip::next(sequence[i - 1]);
      vtAssert(epoch_manip::isRooted(sequence[i]), "Next epoch should be rooted");
    }
  } else /*collective*/ {
    for (auto&& epoch : sequence) {
      epoch = vt::theTerm()->makeEpochCollective();
    }
  }
  return sequence;
}

// fictive distributed computation of a given epoch
void compute(vt::EpochType const& ep) {

  std::random_device device;
  std::mt19937 engine(device());
  std::uniform_int_distribution<int> dist_ttl(1,10);
  std::uniform_int_distribution<int> dist_dest(1,channel::all-1);
  std::uniform_int_distribution<int> dist_round(1,10);

  if(ep != vt::no_epoch){
    debug_print(
      term, node,
      "rank:{}, epoch:{:x}, is_rooted ? {}\n",
      channel::node, ep, epoch::EpochManip::isRooted(ep)
    );
  }

  // process computation for current epoch
  int rounds = dist_round(engine);
  if (channel::all > 2) {
    for (int k = 0; k < rounds; ++k) {
      int dst = channel::node + dist_dest(engine);
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
void finalize(vt::EpochType const& epoch, int order) {

  auto finish = [&](vt::EpochType const& ep) {
    if (ep != vt::no_epoch) {
      vt::theTerm()->finishedEpoch(ep);
    }
  };

  if (epoch == vt::no_epoch) {
    if (channel::node == channel::root) {
      channel::trigger(epoch);
      verify(epoch,order);
    }
  } else if (epoch_manip::isRooted(epoch)) /* real rooted epoch */ {
    if (channel::node == channel::root) {
      using channel::trigger;
      switch(order){
        case 0: { verify(epoch, order); trigger(epoch); finish(epoch); break; }
        case 1: { trigger(epoch); finish(epoch); verify(epoch,order);  break; }
        case 2: { trigger(epoch); verify(epoch,order); finish(epoch);  break; }
        default: vtAbort("Should not be reached");
      }
    }
  } else /* real collective epoch */ {
    finish(epoch);
    verify(epoch, order);
  }

  if (epoch == vt::no_epoch) {
    debug_print(
      term, node,
      "rank:{}, global completed\n",
      channel::node
    );
  } else {
    debug_print(
      term, node,
      "rank:{}, epoch:{:x} completed\n",
      channel::node, epoch
    );
  }
}

void verify(vt::EpochType const& epoch, int order){
  if (channel::node == channel::root) {
    if (epoch == vt::no_epoch) {
      vt::theTerm()->addAction([&]{
        debug_print(
          term, node,
          "rank:{}: no_epoch, order={:x}\n",
          channel::node, order
        );
        // check channel counters
        EXPECT_TRUE(channel::hasEnded(vt::no_epoch));
      });
    } else {
      vt::theTerm()->addAction(epoch, [&]{
        debug_print(
          term, node,
          "rank:{}: epoch={:x}, order={}, is_rooted={}\n",
          channel::node, epoch, order, epoch_manip::isRooted(epoch)
        );
        // check channel counters
        EXPECT_TRUE(channel::hasEnded(epoch));
      });
    }
  }
}

}}}} // end namespace vt::tests::unit::action

#endif /*INCLUDED_TERMINATION_ACTION_COMMON_IMPL_H*/
