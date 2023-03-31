/*
//@HEADER
// *****************************************************************************
//
//                      test_collection_construct_common.h
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

#if !defined INCLUDED_UNIT_COLLECTION_TEST_COLLECTION_CONSTRUCT_COMMON_H
#define INCLUDED_UNIT_COLLECTION_TEST_COLLECTION_CONSTRUCT_COMMON_H

#include "test_parallel_harness.h"
#include "test_collection_common.h"
#include "vt/vrt/collection/manager.h"
#include "data_message.h"

#include <fmt-vt/core.h>
#include <fmt-vt/ostream.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <tuple>
#include <string>
#include <memory>

#define PRINT_CONSTRUCTOR_VALUES 0

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct ConstructHandlers {
  template <
    typename CollectionT,
    typename MessageT = typename CollectionT::MsgType
  >
  static void handler(CollectionT* col, MessageT* msg) {
    // fmt::print(
    //   "{}: constructed TestCol: idx.x()={}\n",
    //   theContext()->getNode(), col->getInex().x(), print_ptr(col)
    // );
  }
};

template <typename CollectionT>
struct TestConstruct : TestParallelHarness {};

template <typename CollectionT>
struct TestConstructDist : TestParallelHarness {};

template <typename ColT>
struct ConstructParams {
  using IndexType = typename ColT::IndexType;
  using ProxyType = CollectionIndexProxy<ColT,IndexType>;

  static ProxyType construct(std::string const& label, IndexType idx) {
    return theCollection()->construct<ColT>(idx, label);
  }

  static ProxyType constructCollective(
    IndexType idx, std::string const& label
  ) {
    return theCollection()->constructCollective<ColT>(
      idx, [=](IndexType my_idx) {
        return std::make_unique<ColT>();
      }, label
    );
  }
};

TYPED_TEST_SUITE_P(TestConstruct);
TYPED_TEST_SUITE_P(TestConstructDist);

template <typename ColT, uint8_t N>
typename ColT::IndexType CreateRange(typename ColT::IndexType::DenseIndexType range) {
  std::array<typename ColT::IndexType::DenseIndexType, N> arr;
  std::fill(arr.begin(), arr.end(), range);

  return arr;
}

template<typename ColType>
void test_construct_1(std::string const& label) {
  using MsgType   = typename ColType::MsgType;

  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    // We don't want too many elements for 4 dimensions
    auto constexpr num_dims = ColType::IndexType::ndims();
    auto constexpr col_size = 8 / num_dims;

    auto rng = CreateRange<ColType, num_dims>(col_size);
    auto proxy = ConstructParams<ColType>::construct(label, rng);
    proxy.template broadcast<
      MsgType,
      ConstructHandlers::handler<ColType,MsgType>
    >();
  }
}

template<typename ColType>
void test_construct_distributed_1() {
  using MsgType   = typename ColType::MsgType;

  // We don't want too many elements for 4 dimensions
  auto constexpr num_dims = ColType::IndexType::ndims();
  auto constexpr col_size = 8 / num_dims;

  auto rng = CreateRange<ColType, num_dims>(col_size);
  auto proxy = ConstructParams<ColType>::constructCollective(
    rng, "test_construct_distributed_1"
  );
  proxy.template broadcast<
    MsgType,
    ConstructHandlers::handler<ColType,MsgType>
  >();
}

}}} // end namespace vt::tests::unit

#endif /*INCLUDED_UNIT_COLLECTION_TEST_COLLECTION_CONSTRUCT_COMMON_H*/
