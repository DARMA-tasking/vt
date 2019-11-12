/*
//@HEADER
// *****************************************************************************
//
//                                 gossiplb.cc
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

#include "vt/config.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/balance/gossiplb/gossiplb.h"
#include "vt/context/context.h"

#include <cstdint>
#include <random>

namespace vt { namespace vrt { namespace collection { namespace lb {

void GossipLB::init(objgroup::proxy::Proxy<GossipLB> in_proxy) {
  proxy = in_proxy;
}

void GossipLB::inputParams(balance::SpecEntry* spec) {
}

void GossipLB::runLB() {
  this->inform();
}

void GossipLB::inform() {
  for (int i = 0; i < f; i++) {
    propagateInfo();
  }
}

void GossipLB::propagateInfo() {
  // First, randomly select a node

  auto const num_nodes = theContext()->getNumNodes();
  std::uniform_int_distribution<NodeType> dist(0, num_nodes - 2);
  std::mt19937 gen(seed());

  dist(gen);
}

void GossipLB::decide() {
  vtAssertExpr(false);
}

void GossipLB::migrate() {
  vtAssertExpr(false);
}

}}}} /* end namespace vt::vrt::collection::lb */
