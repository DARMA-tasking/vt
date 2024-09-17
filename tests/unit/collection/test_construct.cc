/*
//@HEADER
// *****************************************************************************
//
//                              test_construct.cc
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

#include "test_collection_common.h"
#include "test_collection_construct_common.h"

#include <cstdint>
#include <tuple>
#include <string>

namespace vt { namespace tests { namespace unit {

namespace default_ {
struct Col1DMsg;
struct Col4DMsg;

struct TestCol1D : Collection<TestCol1D, vt::Index1D> {
  using MsgType = Col1DMsg;
};

struct TestCol4D : Collection<TestCol4D, vt::IndexND<4>> {
  using MsgType = Col4DMsg;
};

struct Col1DMsg : CollectionMessage<TestCol1D> { };
struct Col4DMsg : CollectionMessage<TestCol4D> { };
} /* end namespace default_ */

using CollectionTestTypes =
  testing::Types<default_::TestCol1D, default_::TestCol4D>;

using CollectionTestDistTypes =
  testing::Types<default_::TestCol1D, default_::TestCol4D>;

TYPED_TEST_P(TestConstruct, test_construct_basic_1) {
  test_construct_1<TypeParam>("test_construct_basic_1");
}

TYPED_TEST_P(TestConstructDist, test_construct_distributed_basic_1) {
  test_construct_distributed_1<TypeParam>();
}

REGISTER_TYPED_TEST_SUITE_P(TestConstruct,     test_construct_basic_1);
REGISTER_TYPED_TEST_SUITE_P(TestConstructDist, test_construct_distributed_basic_1);

INSTANTIATE_TYPED_TEST_SUITE_P(
  test_construct_simple, TestConstruct, CollectionTestTypes, DEFAULT_NAME_GEN
);

INSTANTIATE_TYPED_TEST_SUITE_P(
  test_construct_distributed_simple, TestConstructDist, CollectionTestDistTypes, DEFAULT_NAME_GEN
);

struct TestConstructLabel : TestParallelHarness {};

TEST_F(TestConstructLabel, test_labels) {
  auto const num_nodes = static_cast<int32_t>(theContext()->getNumNodes());
  auto const range = Index1D(num_nodes);
  std::string const label = "test_labels";

  auto proxy = makeCollection<default_::TestCol1D>(label)
    .bounds(range)
    .bulkInsert()
    .wait();

  auto const proxyLabel = theCollection()->getLabel(proxy.getProxy());

  EXPECT_EQ(label, proxyLabel);
}

}}} // end namespace vt::tests::unit
