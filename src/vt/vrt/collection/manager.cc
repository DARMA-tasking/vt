/*
//@HEADER
// *****************************************************************************
//
//                                  manager.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#include "vt/config.h"
#include "vt/configs/arguments/app_config.h"
#include "vt/runtime/runtime.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/base/base.h"
#include "vt/vrt/collection/manager.h"
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"

namespace vt { namespace vrt { namespace collection {

CollectionManager::CollectionManager() { }

void CollectionManager::finalize() {
  cleanupAll<>();
}

/*virtual*/ CollectionManager::~CollectionManager() { }

void CollectionManager::startup() {
#if vt_check_enabled(lblite)
  // First hook, do all LB data manipulation
  thePhase()->registerHookCollective(phase::PhaseHook::End, []{
    auto const& map = theCollection()->collect_lb_data_for_lb_;
    for (auto&& elm : map) {
      // this will trigger all the data collection required for LB
      elm.second();
    }
    auto const cur_phase = thePhase()->getCurrentPhase();
    theLBManager()->selectStartLB(cur_phase);
  });
#endif
}

DispatchBasePtrType
getDispatcher(auto_registry::AutoHandlerType const han) {
  return theCollection()->getDispatcher(han);
}

elm::ElementIDStruct CollectionManager::getCurrentContext() const {
# if vt_check_enabled(lblite)
  if (theContext()->getTask() != nullptr) {
    auto lb = theContext()->getTask()->get<ctx::LBData>();
    if (lb != nullptr) {
      return lb->getCurrentElementID();
    }
  }
#endif
  return elm::ElementIDStruct{};
}

void CollectionManager::schedule(ActionType action) {
  theSched()->enqueue(action);
}

VirtualProxyType CollectionManager::makeCollectionProxy(
  bool is_collective, bool is_migratable
) {
  VirtualIDType const new_id = is_collective ?
    next_collective_id_++ :
    next_rooted_id_++;

  auto const this_node = theContext()->getNode();
  bool const is_collection = true;

  // Create the new proxy with the `new_dist_id`
  auto const proxy = VirtualProxyBuilder::createProxy(
    new_id, this_node, is_collection, is_migratable, is_collective
  );

  vt_debug_print(
    verbose, vrt_coll,
    "makeCollectionProxy: node={}, new_dist_id={}, proxy={:x}\n",
    this_node, new_id, proxy
  );

  return proxy;
}

/*static*/ void CollectionManager::computeReduceStamp(CollectionStampMsg* msg) {
  theCollection()->reduce_stamp_[msg->proxy_] = msg->getVal();
}

}}} /* end namespace vt::vrt::collection */
