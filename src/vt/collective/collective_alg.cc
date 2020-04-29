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

CollectiveScope CollectiveAlg::makeCollectiveScope(TagType in_scope_tag) {
  bool is_user_tag = true;
  auto scope_tag = in_scope_tag;

  if (scope_tag == no_tag) {
    scope_tag = next_system_scope_++;
    is_user_tag = false;
  }

  auto& scopes = is_user_tag ? user_scope_ : system_scope_;

  auto iter = scopes.find(scope_tag);
  vtAssert(iter == scopes.end(), "Scope must not already exist");
  // Must construct in call to make_unique, since constructor is private
  auto ptr = std::make_unique<detail::ScopeImpl>(
    detail::ScopeImpl{is_user_tag, scope_tag}
  );
  scopes[scope_tag] = std::move(ptr);

  return CollectiveScope(is_user_tag, scope_tag);
}

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
  TagType consensus_scope = no_tag;
  TagType consensus_tag = no_tag;
  int consensus_is_user_tag = 0; // bool encoded as an int for MPI

  // The root decides the next tag and tells the remaining nodes
  if (msg->root_ == this_node) {
    auto& scopes = msg->is_user_tag_ ?
      theCollective()->user_scope_ :
      theCollective()->system_scope_ ;

    debug_print(
      gen, node,
      "runCollective: is_user_tag={}, scope={}, seq={}\n",
      msg->is_user_tag_, msg->scope_, msg->seq_
    );

    // Make sure that is exists as it should
    auto iter = scopes.find(msg->scope_);
    vtAssert(iter != scopes.end(), "Collective scope does not exist");

    consensus_scope = msg->scope_;
    consensus_tag = msg->seq_;
    consensus_is_user_tag = msg->is_user_tag_;
  }

  // We can not use a VT broadcast here because it involves the scheduler and
  // MPI progress and there's no safe way to guarantee that the message has been
  // received and processed and doesn't require progress from another node

  int const root = msg->root_;
  auto comm = theContext()->getComm();
  MPI_Request req1, req2, req3;

  // Do a async broadcast of the scope/seq we intend to execute collectively
  // This potentially could be merged into one Ibcast if TagType remains 32-bits
  MPI_Ibcast(&consensus_tag,         1, MPI_INT32_T, root, comm, &req1);
  MPI_Ibcast(&consensus_scope,       1, MPI_INT32_T, root, comm, &req2);
  MPI_Ibcast(&consensus_is_user_tag, 1, MPI_C_BOOL,  root, comm, &req3);

  int flag1 = 0, flag2 = 0, flag3 = 0;
  while (not flag1 or not flag2 or not flag3) {
    if (not flag1) {
      MPI_Test(&req1, &flag1, MPI_STATUS_IGNORE);
    }
    if (not flag2) {
      MPI_Test(&req2, &flag2, MPI_STATUS_IGNORE);
    }
    if (not flag3) {
      MPI_Test(&req3, &flag3, MPI_STATUS_IGNORE);
    }
    runScheduler();
  }

  vtAssert(consensus_tag   != no_tag, "Selected tag must be valid");
  vtAssert(consensus_scope != no_tag, "Selected scope must be valid");

  // We need a barrier here so the root doesn't finish the broadcast first and
  // then enter the collective. We could replace the Ibcast/Ibarrier with a
  // Iallreduce, but I think this is cheaper
  MPI_Ibarrier(comm, &req1);

  flag1 = 0;
  while (not flag1) {
    MPI_Test(&req1, &flag1, MPI_STATUS_IGNORE);
    runScheduler();
  }

  debug_print(
    gen, node,
    "mpiCollective: consensus_tag={}, msg->tag_={}, "
    "consensus_scope={}, msg->scope_={}\n",
    consensus_tag, msg->seq_,
    consensus_scope, msg->scope_
  );

  auto& scopes = consensus_is_user_tag ?
    theCollective()->user_scope_ :
    theCollective()->system_scope_ ;

  // Execute the action that the root selected
  auto iter = scopes.find(consensus_scope);
  vtAssert(iter != scopes.end(), "Collective scope does not exist");
  auto impl = iter->second.get();

  auto iter_seq = impl->planned_collective_.find(consensus_tag);
  vtAssert(
    iter_seq != impl->planned_collective_.end(),
    "Planned collective does not exist within scope"
  );
  auto action = iter_seq->second.action_;

  // Run the collective safely
  action();

  // Erase the tag that was actually executed
  impl->planned_collective_.erase(iter_seq);

  // Cleanup if the CollectiveScope is destroyed and no other planned
  // collectives exists---nothing more can be created
  if (not impl->live_ and impl->planned_collective_.size() == 0) {
    scopes.erase(iter);
  }

  reenter_counter_--;

  // Recurse, running a postponed collective
  auto& postponed = theCollective()->postponed_collectives_;
  if (postponed.size() > 0) {
    auto next_msg = postponed.back();
    postponed.pop_back();
    runCollective(next_msg.get());
  }
}

}}  // end namespace vt::collective
