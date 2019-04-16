/*
//@HEADER
// ************************************************************************
//
//                   test_reduce_collection_common.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "data_message.h"
#include "vt/transport.h"

#if !defined INCLUDED_TEST_REDUCE_COLLECTION_COMMON_H
#define INCLUDED_TEST_REDUCE_COLLECTION_COMMON_H

namespace vt { namespace tests { namespace unit { namespace reduce {

int const collect_size = 32;
int const index_tresh  = 8;

struct MyReduceMsg : vt::collective::ReduceMsg {
  MyReduceMsg(int const& in_num)
    : num(in_num) {}

  int num = 0;
};

struct MyReduceNoneMsg : vt::collective::ReduceNoneMsg { };

struct VectorPayload {
  VectorPayload() = default;

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

struct SysMsgVec : vt::collective::ReduceTMsg<VectorPayload> {
  SysMsgVec() = default;

  explicit SysMsgVec(double in_num)
    : ReduceTMsg<VectorPayload>() {
    getVal().vec.push_back(in_num);
    getVal().vec.push_back(in_num + 1);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    ReduceTMsg<VectorPayload>::invokeSerialize(s);
  }
};

struct CheckVec {
  void operator()(SysMsgVec* msg) {
    auto const size = msg->getConstVal().vec.size();
    debug_print(reduce, node, "final vec.size={}\n", size);
    EXPECT_EQ(size, reduce::collect_size * 2U);
  }
};

struct NoneReduce {
  void operator()(MyReduceNoneMsg* msg) { }
};

struct MyCol : Collection<MyCol, Index1D> {
  MyCol()
    : Collection<MyCol, Index1D>() {
    debug_print(
      reduce, node
      "constructing MyCol on node={}: idx.x()={}, ptr={}\n",
      theContext()->getNode(), getIndex().x(), print_ptr(this)
    );
  }

  virtual ~MyCol() = default;
};

struct ColMsg : CollectionMessage<MyCol> {
  NodeType from_node;

  ColMsg() = default;

  explicit ColMsg(NodeType const& in_from_node)
    : from_node(in_from_node)
  {}
};

template <int expected, bool check = true>
void reducePlus(MyReduceMsg* msg) {
  debug_print(
    reduce, node,
    "reducePlus: cur={}: is_root={}, count={}, next={}, num={}\n",
    print_ptr(msg), print_bool(msg->isRoot()),
    msg->getCount(), print_ptr(msg->getNext<MyReduceMsg>()), msg->num
  );

  if (msg->isRoot()) {
    int const value = msg->num;
    debug_print(reduce, node, "reducePlus: final num={}\n", value);
    if (check) {
      EXPECT_EQ(value, expected);
    }
  } else {
    auto fst_msg = msg;
    auto cur_msg = msg->getNext<MyReduceMsg>();
    while (cur_msg != nullptr) {
      debug_print(
        reduce, node,
        "reducePlus: while fst_msg={}: cur_msg={}, is_root={}, count={}, next={}, num={}\n",
        print_ptr(fst_msg), print_ptr(cur_msg), print_bool(cur_msg->isRoot()),
        cur_msg->getCount(), print_ptr(cur_msg->getNext<MyReduceMsg>()), cur_msg->num
      );

      fst_msg->num += cur_msg->num;
      cur_msg = cur_msg->getNext<MyReduceMsg>();
    }
  }
}

}}}} // end namespace vt::tests::unit::reduce
#endif /*INCLUDED_TEST_REDUCE_COLLECTION_COMMON_H*/
