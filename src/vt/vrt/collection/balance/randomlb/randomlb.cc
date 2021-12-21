/*
//@HEADER
// *****************************************************************************
//
//                                 randomlb.cc
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

#include "vt/vrt/collection/balance/randomlb/randomlb.h"
#include "vt/vrt/collection/balance/model/load_model.h"

#include <random>
#include <set>

namespace vt { namespace vrt { namespace collection { namespace lb {

void RandomLB::init(objgroup::proxy::Proxy<RandomLB> in_proxy) {
  proxy = in_proxy;
}

/*static*/ std::unordered_map<std::string, std::string>
RandomLB::getInputKeysWithHelp() {
  std::unordered_map<std::string, std::string> const keys_help = {
    {
      "randomize_seed",
      R"(
Values: {true, false}
Default: false
Description:
  Whether the seed which determines placement of objects should be randomized.
  The parameter "seed" is only read if this is set to false. When it is true,
  the load balancer will create a std::random_device to generate the values.
)"
    },
    {
      "seed",
      R"(
Values: <int>
Default: 123456789
Description:
  An integer value that is used to seed the value that generates random
  locations for migrations. The seed is deterministically combined with the
  phase number and the node (rank) to provide a unique seed for each rank for
  each phase.
)"
    }
  };
  return keys_help;
}

void RandomLB::inputParams(balance::SpecEntry* spec) {
  auto keys_help = getInputKeysWithHelp();

  std::vector<std::string> allowed;
  for (auto&& elm : keys_help) {
    allowed.push_back(elm.first);
  }
  spec->checkAllowedKeys(allowed);
  seed_ = spec->getOrDefault<int32_t>("seed", seed_);
  randomize_seed_ = spec->getOrDefault<bool>("randomize_seed", randomize_seed_);
}

void RandomLB::runLB(TimeType) {
  auto const this_node = theContext()->getNode();
  auto const num_nodes = static_cast<int32_t>(theContext()->getNumNodes());

  if (this_node == 0) {
    vt_print(
      lb, "RandomLB: runLB: randomize_seed={}, seed={}\n",
      randomize_seed_, seed_
    );
    fflush(stdout);
  }

  std::mt19937 gen;
  if (randomize_seed_) {
    std::random_device rd;
    gen = std::mt19937{rd()};
  } else {
    using ResultType = std::mt19937::result_type;
    auto const node_seed = seed_ + this->phase_ + static_cast<ResultType>(this_node);
    gen = std::mt19937{static_cast<ResultType>(node_seed)};
  }
  std::uniform_int_distribution<> dist(0, num_nodes-1);

  // Sort the objects so we have a deterministic order over them
  std::set<ObjIDType> objs;
  for (auto obj : *load_model_) {
    if (obj.migratable) {
      objs.insert(obj);
    }
  }

  // we skip the first object to be certain we never end up with zero objects
  for (auto it = ++objs.begin(); it != objs.end(); ++it) {
    auto const to_node = dist(gen);
    if (to_node != this_node) {
      vt_debug_print(
        terse, lb,
        "RandomLB: migrating obj={:x} home={} from={} to={}\n",
        it->id, it->home_node, this_node, to_node
      );
      migrateObjectTo(*it, to_node);
    }
  }
}

}}}} /* end namespace vt::vrt::collection::balance::lb */

