/*
//@HEADER
// *****************************************************************************
//
//                            test_location_indep.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#include "test_parallel_harness.h"
#include "vt/topos/location/manager.h"

namespace vt::tests::unit::location {

using TestLocationIndep = TestParallelHarness;

TEST_F(TestLocationIndep, test_create_locman) {
  using EntityType = uint64_t;

  auto lm_proxy = vt::theLocMan()->createCollective<EntityType>(
    false /* no anytime migration */,
    true /* keep cache up-to-date */,
    256 /* size of cache */
  );

  vt::theLocMan()->destroyCollective(lm_proxy);
}

TEST_F(TestLocationIndep, test_register_entity_locman) {
  using EntityType = uint64_t;

  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  auto lm_proxy = vt::theLocMan()->createCollective<EntityType>(
    false /* no anytime migration */,
    true /* keep cache up-to-date */,
    256 /* size of cache */
  );

  for (int i = 0; i < num_nodes; i++) {
    auto const elm = num_nodes * this_node + i;
    vt_print(gen, "registering i={} home={}\n", elm, this_node);
    lm_proxy.get()->registerEntity(elm, this_node);
  }

  runInEpochCollective("getLocation", [&]{
    for (int i = 0; i < num_nodes*num_nodes; i++) {
      lm_proxy.get()->getLocation(i, i/num_nodes, [=](NodeType found) {
        vt_print(gen, "found node: i={} node={}\n", i, found);
      });
    }
  });
  
  vt::theLocMan()->destroyCollective(lm_proxy);
}


} /* end namespace vt::tests::unit::location */
