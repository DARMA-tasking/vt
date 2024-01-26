/*
//@HEADER
// *****************************************************************************
//
//                       test_reduce_collection_common.h
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

#if !defined INCLUDED_UNIT_COLLECTION_TEST_REDUCE_COLLECTION_COMMON_H
#define INCLUDED_UNIT_COLLECTION_TEST_REDUCE_COLLECTION_COMMON_H

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "data_message.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace tests { namespace unit { namespace reduce {

int const collect_size = 32;
int const index_tresh  = 8;

struct VectorPayload {
  VectorPayload() = default;
  VectorPayload(double num) {
    vec.push_back(num);
    vec.push_back(num + 1);
  }

  friend VectorPayload operator+(VectorPayload v1, VectorPayload const& v2) {
    for (auto&& elm : v2.vec) {
      v1.vec.push_back(elm);
    }
    return v1;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | vec;
  }

  std::vector<double> vec;
};

struct MyCol : Collection<MyCol, Index1D> {
  MyCol() {
    vt_debug_print(
      normal, reduce,
      "constructing MyCol on node={}: idx.x()={}, ptr={}\n",
      theContext()->getNode(), getIndex().x(), print_ptr(this)
    );
  }

  void noneReduce() { }

  void checkVec(VectorPayload v) {
    auto const size = v.vec.size();
    vt_debug_print(normal, reduce, "final vec.size={}\n", size);
    EXPECT_EQ(size, reduce::collect_size * 2U);
  }

  void checkNum(int val) {
    EXPECT_EQ(val, collect_size * (collect_size - 1) / 2);
  }

  virtual ~MyCol() = default;
};

struct ColMsg : CollectionMessage<MyCol> {
  NodeT from_node;

  ColMsg() = default;

  explicit ColMsg(NodeT const& in_from_node)
    : from_node(in_from_node)
  {}
};

}}}} // end namespace vt::tests::unit::reduce

#endif /*INCLUDED_UNIT_COLLECTION_TEST_REDUCE_COLLECTION_COMMON_H*/
