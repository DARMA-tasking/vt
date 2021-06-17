/*
//@HEADER
// *****************************************************************************
//
//                             collective_scope.cc
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

#if !defined INCLUDED_VT_COLLECTIVE_COLLECTIVE_SCOPE_CC
#define INCLUDED_VT_COLLECTIVE_COLLECTIVE_SCOPE_CC

#include "vt/config.h"
#include "vt/collective/collective_scope.h"
#include "vt/collective/collective_alg.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/scheduler/scheduler.h"

namespace vt { namespace collective {

TagType CollectiveScope::mpiCollectiveAsync(ActionType action) {
  auto impl = getScope();
  vtAssert(impl != nullptr, "Must have a valid, allocated scope");
  auto tag = impl->next_seq_++;
  auto epoch = theMsg()->getEpoch();

  // Create a new collective action with the next tag
  detail::ScopeImpl::CollectiveInfo info(tag, action, epoch);
  theTerm()->produce(epoch);

  impl->planned_collective_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(tag),
    std::forward_as_tuple(info)
  );

  vt_debug_print(
    terse, gen,
    "mpiCollectiveAsync: is_user_tag_={}, scope={}: new MPI collective with "
    "seq tag={}\n",
    is_user_tag_, scope_, tag
  );

  // Do a reduction followed by a broadcast to trigger a collective
  // operation. Note that in VT reductions and broadcasts can be executed out of
  // order. This implies that runCollective might be called with different tags
  // on different nodes. Thus, in runCollective, we will use a consensus
  // protocol to agree on a consistent tag across all the nodes.
  NodeType collective_root = 0;

  using CollectiveMsg = CollectiveAlg::CollectiveMsg;
  auto cb = theCB()->makeBcast<CollectiveMsg,&CollectiveAlg::runCollective>();
  auto msg = makeMessage<CollectiveMsg>(is_user_tag_, scope_, tag, collective_root);

  // The tag for the reduce is a combination of the scope and seq tag.
  auto r = theCollective()->reducer();

  using collective::reduce::makeStamp;
  using collective::reduce::TagPair;

  auto stamp = makeStamp<TagPair>(scope_, tag);
  r->reduce<collective::None>(collective_root, msg.get(), cb, stamp);

  return tag;
}

bool CollectiveScope::isCollectiveDone(TagType tag) {
  auto impl = getScope();
  vtAssert(impl != nullptr, "Must have a valid, allocated scope");
  return impl->planned_collective_.find(tag) == impl->planned_collective_.end();
}

void CollectiveScope::waitCollective(TagType tag) {
  theSched()->runSchedulerWhile([this,tag]{ return not isCollectiveDone(tag); });
}

void CollectiveScope::mpiCollectiveWait(ActionType action) {
  waitCollective(mpiCollectiveAsync(action));
}

detail::ScopeImpl* CollectiveScope::getScope() {
  if (scope_ == no_tag) {
    return nullptr;
  }

  auto& scopes = is_user_tag_ ?
    theCollective()->user_scope_ :
    theCollective()->system_scope_;

  auto scope_iter = scopes.find(scope_);
  vtAssert(scope_iter != scopes.end(), "Scope must exist");
  return scope_iter->second.get();
}

CollectiveScope::~CollectiveScope() {
  auto impl = getScope();
  if (impl != nullptr) {
    impl->live_ = false;

    if (impl->planned_collective_.size() == 0) {
      auto& scopes = is_user_tag_ ?
        theCollective()->user_scope_ :
        theCollective()->system_scope_;

      auto scope_iter = scopes.find(scope_);
      if (scope_iter != scopes.end()) {
        scopes.erase(scope_iter);
      }
    }
  }
}

}} /* end namespace vt::collective */

#endif /*INCLUDED_VT_COLLECTIVE_COLLECTIVE_SCOPE_CC*/
