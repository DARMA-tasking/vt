/*
//@HEADER
// *****************************************************************************
//
//                         test_allreduce_collection.cc
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


#include "test_parallel_harness.h"
#include "vt/collective/reduce/allreduce/type.h"
#include "vt/collective/reduce/operators/functors/max_op.h"
#include "vt/scheduler/scheduler.h"
#include "vt/vrt/collection/manager.h"
#include "vt/topos/index/index.h"
#include <algorithm>
#include <fmt-vt/core.h>
#include <gtest/gtest.h>

namespace vt { namespace tests { namespace unit {

struct RecursiveDoublingColl : vt::Collection<RecursiveDoublingColl, vt::Index1D> {
  template <size_t size, int32_t num_elms_per_node>
  void sumAllreduceHan(std::vector<int32_t> result) {
    ++counter_;

    ASSERT_EQ(result.size(), size);

    auto const num_nodes = theContext()->getNumNodes();
    auto const num_elems = num_nodes * num_elms_per_node;
    auto const expected_val = ((num_elems - 1) * num_elems) / 2;

    auto verify_result =
      std::all_of(result.begin(), result.end(), [=](auto const& val) {
        return val == expected_val;
      });

    ASSERT_TRUE(verify_result);
  }

  template <size_t size, int32_t num_elms_per_node>
  void maxAllreduceHan(std::vector<int32_t> result) {
    ++counter_;

    ASSERT_EQ(result.size(), size);

    auto const num_nodes = theContext()->getNumNodes();
    auto const expected_val = (num_nodes * num_elms_per_node) - 1;

    auto verify_result = std::all_of(result.begin(), result.end(), [=](auto const& val) {
      return val == expected_val;
    });

    ASSERT_TRUE(verify_result);
  }

  template <typename ReducerT, size_t size, int32_t num_elms_per_node>
  void executePlusAllreduce() {
    using namespace collective::reduce::allreduce;
    auto proxy = this->getCollectionProxy();

    std::vector<int32_t> payload(size, getIndex().x());
    proxy.allreduce<
      ReducerT, &RecursiveDoublingColl::sumAllreduceHan<size, num_elms_per_node>,
      collective::PlusOp
    >(payload);
  }

  template <typename ReducerT, size_t size, int32_t num_elms_per_node>
  void executeMaxAllreduce() {
    using namespace collective::reduce::allreduce;
    auto proxy = this->getCollectionProxy();

    std::vector<int32_t> payload(size, getIndex().x());
    proxy.allreduce<
      ReducerT, &RecursiveDoublingColl::maxAllreduceHan<size, num_elms_per_node>,
      collective::MaxOp
    >(payload);
  }

  int32_t counter_ = 0;
};

struct TestAllreduceCollection : TestParallelHarness {};

TEST_F(TestAllreduceCollection, test_allreduce_recursive_doubling) {
  using namespace vt::collective::reduce::allreduce;

  auto const my_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  constexpr auto num_elms_per_node = 3;
  auto range = vt::Index1D(int32_t{num_nodes * num_elms_per_node});
  auto proxy = vt::makeCollection<RecursiveDoublingColl>("test_collection_allreduce")
                 .bounds(range)
                 .bulkInsert()
                 .wait();

  constexpr size_t size = 100;
  auto const elm = my_node * num_elms_per_node;
  auto const& counter = proxy[elm].tryGetLocalPtr()->counter_;

  vt::runInEpochCollective([=] {
    proxy.broadcastCollective<
      &RecursiveDoublingColl::executePlusAllreduce<RecursiveDoublingT, size, num_elms_per_node>
    >();
  });

  ASSERT_EQ(counter, 1);

  vt::runInEpochCollective([=] {
    proxy.broadcastCollective<
      &RecursiveDoublingColl::executeMaxAllreduce<RecursiveDoublingT, size, num_elms_per_node>
    >();
  });

  ASSERT_EQ(counter, 2);

  vt::runInEpochCollective([=] {
    proxy.broadcastCollective<
      &RecursiveDoublingColl::executePlusAllreduce<RabenseifnerT, size, num_elms_per_node>
    >();
  });

  ASSERT_EQ(counter, 3);

  vt::runInEpochCollective([=] {
    proxy.broadcastCollective<
      &RecursiveDoublingColl::executeMaxAllreduce<RabenseifnerT, size, num_elms_per_node>
    >();
  });

  ASSERT_EQ(counter, 4);
}

}}} // end namespace vt::tests::unit
