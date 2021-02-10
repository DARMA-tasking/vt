/*
//@HEADER
// *****************************************************************************
//
//                             subphase_manager.cc
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

#include "vt/phase/subphase/subphase_manager.h"
#include "vt/utils/bits/bits_packer.h"
#include "vt/objgroup/manager.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/phase/subphase/subphase_msgs.h"
#include "vt/phase/subphase/subphase_bits.h"
#include "vt/phase/phase_manager.h"

#include <functional>

namespace vt { namespace phase { namespace subphase {

/*static*/ void SubphaseManager::subphaseConstruct(SubphaseManager* ptr) {
  auto proxy = theObjGroup()->makeCollective<SubphaseManager>(ptr);
  proxy.get()->subphase_proxy_ = proxy.getProxy();
}

SubphaseType SubphaseManager::registerCollectiveSubphase(
  std::string const& label
) {
  // Generator turn the raw hashed bits into a ID that doesn't collide with
  // rooted IDs by setting the top bit---see enum layout \c eSubphaseLayout
  auto generator = [](SubphaseType raw_id) -> SubphaseType {
    return SubphaseBits::makeID(true, 0, raw_id);
  };

  // Check if we have already generated a ID for this collective subphase
  auto iter = collective_ids_.find(label);
  if (iter != collective_ids_.end()) {
    return iter->second;
  }

  // Hash the label
  std::size_t h = std::hash<std::string>{}(label);
  SubphaseType final_id = generator(h);

  // Check for any conflicts with other collective IDs; loop until we don't have
  // a conflict
  uint64_t offset = 2;
  while (collective_used_ids_.find(final_id) != collective_used_ids_.end()) {
    // exponential backoff while there is a hash collision
    h += offset;
    offset <<= 1;
    final_id = generator(h);
  };

  vtAssert(
    collective_used_ids_.find(final_id) == collective_used_ids_.end(),
    "Must not exist already"
  );

  collective_used_ids_.insert(final_id);
  collective_ids_[label] = final_id;
  return final_id;
}

void SubphaseManager::registerRootedSubphase(
  std::string const& label, SubphaseAction action
) {
  vt_debug_print(
    phase, node,
    "SubphaseManager::registerRootedSubphase: label={}\n", label
  );

  // Check if it's pending.. if so push it back; we are awaiting resolution
  auto iter = pending_.find(label);
  if (iter != pending_.end()) {
    pending_actions_[label].push_back(action);
    return;
  } else {
    // Insert pending so we don't perform the action again
    pending_.insert(label);
  }

  // Hash to find out the broker which will generate the universal ID
  NodeType const num_nodes = theContext()->getNumNodes();
  std::size_t const h = std::hash<std::string>{}(label);
  NodeType const broker = static_cast<NodeType>(h % num_nodes);

  vt_debug_print(
    phase, node,
    "SubphaseManager::registerRootedSubphase: label={}, hash={}, "
    "contacting broker={}\n",
    label, h, broker
  );

  // Callback when the broker resolves the ID
  auto cb = theCB()->makeFunc<SubphaseIDMsg>(
    pipe::LifetimeEnum::Once,
    [label,action,this](SubphaseIDMsg* sub_msg) {
      auto const id = sub_msg->id_;
      vtAssert(rooted_ids_.find(label) == rooted_ids_.end(), "Must not exist");
      rooted_ids_[label] = id;

      vt_debug_print(
        phase, node,
        "SubphaseManager::registerRootedSubphase: callback: label={}, "
        "new id={}\n",
        label, id
      );

      // Fire all the actions and clean up
      action(id);
      auto res_id = pending_actions_.find(label);
      if (res_id != pending_actions_.end()) {
        for (auto&& c : res_id->second) {
          c(id);
        }
        pending_actions_.erase(res_id);
      }
      auto pending_iter = pending_.find(label);
      vtAssert(pending_iter != pending_.end(), "Must be in pending");
      pending_.erase(pending_iter);
    }
  );

  // Send to the broker for resolution
  objgroup::proxy::Proxy<SubphaseManager> proxy{subphase_proxy_};
  proxy[broker].send<RootedStringMsg, &SubphaseManager::resolveRootedString>(
    label, cb
  );
}

void SubphaseManager::resolveRootedString(RootedStringMsg* msg) {
  auto const& label = msg->subphase_;
  auto cb = msg->cb_;
  SubphaseType id = no_lb_phase;

  vt_debug_print(
    phase, node,
    "SubphaseManager::resolveRootedString: handler: label={}, found={}\n",
    label, resolved_broker_ids_.find(label) != resolved_broker_ids_.end()
  );

  // Check to see if we generated an ID for this already---if so, send that
  // back!
  auto iter = resolved_broker_ids_.find(label);
  if (iter != resolved_broker_ids_.end()) {
    id = iter->second;
  } else {
    auto seq_id = broker_rooted_id_;
    broker_rooted_id_++;

    // Generate a new unique ID
    auto const n = theContext()->getNode();
    id = SubphaseBits::makeID(false, n, seq_id);

    // Save the ID for future requests
    resolved_broker_ids_[label] = id;
  }

  vt_debug_print(
    phase, node,
    "SubphaseManager::resolveRootedString: handler: label={}, gen id={}\n",
    label, id
  );

  // Send it back to the requesting node
  cb.send(id);
}

void SubphaseManager::reduceLabels(
  std::function<void(IDMapType const& map)> callback
) {
  auto cb = theCB()->makeFunc<StringIDMapMsg>(
    pipe::LifetimeEnum::Once,
    [callback](StringIDMapMsg* msg) { callback(msg->getVal().map_); }
  );

  IDMapType local_map = collective_ids_;
  for (auto&& elm : resolved_broker_ids_) {
    local_map[elm.first] = elm.second;
  }

  auto msg = makeMessage<StringIDMapMsg>(local_map);

  NodeType const cb_root = 0;

  auto r = thePhase()->reducer();
  r->reduce<collective::PlusOp<StringIDMap>>(cb_root, msg.get(), cb);
}

bool SubphaseManager::isPendingResolution() const {
  return pending_.size() > 0;
}

}}} /* end namespace vt::phase::subphase */
