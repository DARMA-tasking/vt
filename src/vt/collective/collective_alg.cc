/*
//@HEADER
// *****************************************************************************
//
//                              collective_alg.cc
//                           DARMA Toolkit v. 1.0.0
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

#include "vt/collective/collective_alg.h"

namespace vt { namespace collective {

CollectiveAlg::CollectiveAlg()
  : tree::Tree(tree::tree_cons_tag_t),
    reduce::Reduce(),
    barrier::Barrier()
{ }

/*static*/ void CollectiveAlg::runCollective(CollectiveMsg* msg) {
  // We need a reentrancy counter to ensure that this is only on the scheduler
  // stack once!
  static int reenter_counter_ = 0;

  if (reenter_counter_ != 0) {
    // Postpone any collective while the current one resolves
    theCollective()->postponed_collectives_.emplace_back(promoteMsg(msg));
    return;
  }

  reenter_counter_++;

  auto const this_node = theContext()->getNode();
  TagType consensus_tag = no_tag;
  ActionType action = no_action;

  auto& planned = theCollective()->planned_collective_;

  // The root decides the next tag and tells the remaining nodes
  if (msg->root_ == this_node) {
    debug_print(
      gen, node,
      "mpiCollective: running collective associated with tag={}\n",
      msg->tag_
    );

    auto iter = planned.find(msg->tag_);
    vtAssert(iter != planned.end(), "Planned collective does not exist");
    consensus_tag = msg->tag_;
    action = iter->second.action_;
  }

  // We can not use a VT broadcast here because it involves the scheduler and
  // MPI progress and there's no safe way to guarantee that the message has been
  // received and processed and doesn't require progress from another node

  int const root = msg->root_;
  auto comm = theContext()->getComm();
  MPI_Request req;

  // Do a async broadcast of the tag we intend to execute collectively
  MPI_Ibcast(&consensus_tag, 1, MPI_INT32_T, root, comm, &req);

  int flag = 0;
  while (not flag) {
    MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
    runScheduler();
  }

  // We need a barrier here so the root doesn't finish the broadcast first and
  // then enter the collective. We could replace the Ibcast/Ibarrier with a
  // Iallreduce, but I think this is cheaper
  MPI_Ibarrier(comm, &req);

  flag = 0;
  while (not flag) {
    MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
    runScheduler();
  }

  debug_print(
    gen, node,
    "mpiCollective: consensus_tag={}, msg->tag_={}\n",
    consensus_tag, msg->tag_
  );

  // The root had a different collective in mind... let's do that one
  if (msg->root_ != this_node) {
    auto iter = planned.find(consensus_tag);
    vtAssert(iter != planned.end(), "Planned collective does not exist");
    action = iter->second.action_;
  }

  // Run the collective safely
  action();

  // Erase the tag that was actually executed
  planned.erase(planned.find(consensus_tag));

  reenter_counter_--;

  // Recurse, running a postponed collective
  auto& postponed = theCollective()->postponed_collectives_;
  if (postponed.size() > 0) {
    auto next_msg = postponed.back();
    postponed.pop_back();
    runCollective(next_msg.get());
  }
}


TagType CollectiveAlg::mpiCollectiveAsync(ActionType action) {
  auto tag = next_tag_++;

  // Create a new collective action with the next tag
  CollectiveInfo info(tag, action);

  planned_collective_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(tag),
    std::forward_as_tuple(info)
  );

  debug_print(
    gen, node,
    "mpiCollectiveAsync: new MPI collective with tag={}\n",
    tag
  );

  // Do a reduction followed by a broadcast to trigger a collective
  // operation. Note that in VT reductions and broadcasts can be executed out of
  // order. This implies that runCollective might be called with different tags
  // on different nodes. Thus, in runCollective, we will use a consensus
  // protocol to agree on a consistent tag across all the nodes.
  NodeType collective_root = 0;

  auto cb = theCB()->makeBcast<CollectiveMsg,&runCollective>();

  // Put this in a separate namespace
  // @todo: broader issue: need to implement a better way to scope reductions
  auto ident = 0xEFFFFFFFFFFFFFFF;
  auto msg = makeMessage<CollectiveMsg>(tag, collective_root);
  this->reduce<collective::None>(
    collective_root, msg.get(), cb, tag, no_seq_id, 1, ident
  );

  return tag;
}

bool CollectiveAlg::isCollectiveDone(TagType tag) {
  return planned_collective_.find(tag) == planned_collective_.end();
}

void CollectiveAlg::waitCollective(TagType tag) {
  while (not isCollectiveDone(tag)) {
    runScheduler();
  }
}

void CollectiveAlg::mpiCollectiveWait(ActionType action) {
  waitCollective(mpiCollectiveAsync(action));
}

}}  // end namespace vt::collective
