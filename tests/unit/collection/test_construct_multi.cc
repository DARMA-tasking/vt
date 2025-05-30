/*
//@HEADER
// *****************************************************************************
//
//                           test_construct_multi.cc
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"

#include <vt/transport.h>

#include <cstdint>
#include <tuple>
#include <string>

namespace vt { namespace tests { namespace unit {

struct TestConstructMulti : TestParallelHarness {};

namespace multi_ {
struct TestCol1D : Collection<TestCol1D, vt::Index1D> { };
} /* end namespace multi_ */

TEST_F(TestConstructMulti, test_multi_construct) {
  auto const this_node = theContext()->getNode();
  auto const num_nodes = static_cast<int32_t>(theContext()->getNumNodes());
  auto const range = Index1D(num_nodes);
  std::string const label = "test_multi_construct";

  for (int j = 0; j < 4; j++) {
    vt::runInEpochCollective([&]{
      if (this_node == 1) {
        for (int i = 0; i < 4; i++) {
          auto proxy = makeCollection<multi_::TestCol1D>(label)
            .bounds(range)
            .bulkInsert()
            .collective(false)
            .wait();
          (void)proxy;
        }
      }
    });
  }
}

}}} // end namespace vt::tests::unit
