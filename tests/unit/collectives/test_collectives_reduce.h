/*
//@HEADER
// *****************************************************************************
//
//                          test_collectives_reduce.h
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

#if !defined INCLUDED_UNIT_COLLECTIVES_TEST_COLLECTIVES_REDUCE_H
#define INCLUDED_UNIT_COLLECTIVES_TEST_COLLECTIVES_REDUCE_H

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/collective/collective.h"
#include "vt/collective/collective_alg.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::collective;
using namespace vt::tests::unit;

enum struct ReduceOP : int {
  Plus = 0,
  Max  = 1,
  Min  = 2
};

struct MyReduceMsg : ReduceMsg {
  MyReduceMsg(int in_num)
    : num(in_num)
  {}

  int num = 0;
};

struct SysMsg : ReduceTMsg<int> {
  explicit SysMsg(int in_num)
    : ReduceTMsg<int>(in_num)
  {}
};

struct TestReduce : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;

  static void reducePlus(MyReduceMsg* msg) {
    vt_debug_print(
      normal, reduce,
      "cur={}: is_root={}, count={}, next={}, num={}\n",
      print_ptr(msg), print_bool(msg->isRoot()), msg->getCount(),
      print_ptr(msg->getNext<MyReduceMsg>()), msg->num
    );

    if (msg->isRoot()) {
      vt_debug_print(normal, reduce, "final value={}\n", msg->num);
      auto n = vt::theContext()->getNumNodes();
      // check expected result
      EXPECT_EQ(msg->num, n * (n - 1)/2);
    } else {
      auto fst_msg = msg;
      auto cur_msg = msg->getNext<MyReduceMsg>();

      while (cur_msg not_eq nullptr) {
        vt_debug_print(
          normal, reduce,
          "while fst_msg={}: cur_msg={}, is_root={}, count={}, next={}, num={}\n",
          print_ptr(fst_msg), print_ptr(cur_msg), print_bool(cur_msg->isRoot()),
          cur_msg->getCount(), print_ptr(cur_msg->getNext<MyReduceMsg>()),
          cur_msg->num
        );

        fst_msg->num += cur_msg->num;
        cur_msg = cur_msg->getNext<MyReduceMsg>();
      }
    }
  }
};

template <ReduceOP oper>
struct Verify {

  void operator()(SysMsg* msg) {
    // print value
    auto value = msg->getConstVal();
    vt_debug_print(normal, reduce, "final value={}\n", value);

    // check result
    auto n = vt::theContext()->getNumNodes();

    switch (oper) {
      case ReduceOP::Plus: EXPECT_EQ(value, n * (n - 1)/2); break;
      case ReduceOP::Min:  EXPECT_EQ(value, 0); break;
      case ReduceOP::Max:  EXPECT_EQ(value, n - 1); break;
      default: vtAbort("Failure: should not be reached"); break;
    }
  }
  void operator()(ReduceVecMsg<bool>* msg) {
    auto value = msg->getConstVal();

    EXPECT_EQ(value.size(), static_cast<std::size_t>(2));
    EXPECT_EQ(value[0], false);
    EXPECT_EQ(value[1], true);
  }

  void operator()(ReduceVecMsg<int>* msg) {
    auto value = msg->getConstVal();

    auto n = vt::theContext()->getNumNodes();

    for (std::size_t i = 0; i < value.size(); ++i) {
      EXPECT_EQ(value[i], static_cast<int>(i * n));
    }
  }
};

}}} // end namespace vt::tests::unit

#endif /*INCLUDED_UNIT_COLLECTIVES_TEST_COLLECTIVES_REDUCE_H*/
