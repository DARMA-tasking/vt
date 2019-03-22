/*
//@HEADER
// ************************************************************************
//
//                          test_collectives_reduce.cc
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

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::collective;
using namespace vt::tests::unit;

struct MyReduceMsg : ReduceMsg {
  MyReduceMsg(int const& in_num)
    : num(in_num)
  { }

  int num = 0;
};

struct SysMsg : ReduceTMsg<int> {
  explicit SysMsg(int in_num)
    : ReduceTMsg<int>(in_num)
  { }
};

struct Print {
  void operator()(SysMsg* msg) {
    fmt::print("final value={}\n", msg->getConstVal());
  }
};

struct TestReduce : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;

  virtual void SetUp() {
    TestParallelHarness::SetUp();
  }

  static void reducePlus(MyReduceMsg* msg) {
    fmt::print(
      "cur={}: is_root={}, count={}, next={}, num={}\n",
      print_ptr(msg), print_bool(msg->isRoot()), msg->getCount(),
      print_ptr(msg->getNext<MyReduceMsg>()), msg->num
    );

    if (msg->isRoot()) {
      fmt::print("final num={}\n", msg->num);
    } else {
      MyReduceMsg* fst_msg = msg;
      MyReduceMsg* cur_msg = msg->getNext<MyReduceMsg>();
      while (cur_msg != nullptr) {
        fmt::print(
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

TEST_F(TestReduce, test_reduce_op) {
  auto const& my_node = theContext()->getNode();
  auto const& root = 0;

  MyReduceMsg* msg = makeSharedMessage<MyReduceMsg>(my_node);
  fmt::print("msg->num={}\n", msg->num);
  theCollective()->reduce<MyReduceMsg, reducePlus>(root, msg);
}

TEST_F(TestReduce, test_reduce_plus_default_op) {
  auto const& my_node = theContext()->getNode();
  auto const& root = 0;

  auto msg = makeSharedMessage<SysMsg>(my_node);
  fmt::print("msg->num={}\n", msg->getConstVal());
  theCollective()->reduce<
    SysMsg,
    SysMsg::msgHandler<SysMsg,PlusOp<int>,Print>
  >(root, msg);
}

TEST_F(TestReduce, test_reduce_max_default_op) {
  auto const& my_node = theContext()->getNode();
  auto const& root = 0;

  auto msg = makeSharedMessage<SysMsg>(my_node);
  fmt::print("msg->num={}\n", msg->getConstVal());
  theCollective()->reduce<
    SysMsg,
    SysMsg::msgHandler<SysMsg,MaxOp<int>,Print>
  >(root, msg);
}

TEST_F(TestReduce, test_reduce_min_default_op) {
  auto const& my_node = theContext()->getNode();
  auto const& root = 0;

  auto msg = makeSharedMessage<SysMsg>(my_node);
  fmt::print("msg->num={}\n", msg->getConstVal());
  theCollective()->reduce<
    SysMsg,
    SysMsg::msgHandler<SysMsg,MinOp<int>,Print>
  >(root, msg);
}

}}} // end namespace vt::tests::unit
