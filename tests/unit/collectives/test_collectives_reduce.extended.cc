/*
//@HEADER
// *****************************************************************************
//
//                      test_collectives_reduce.extended.cc
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "data_message.h"
#include "test_collectives_reduce.h"

#include "vt/collective/collective.h"
#include "vt/objgroup/manager.h"

namespace vt { namespace tests { namespace unit {

TEST_F(TestReduce, test_reduce_plus_default_op) {
  auto const my_node = theContext()->getNode();
  auto const root = 0;

  auto msg = makeMessage<SysMsg>(my_node);
  vt_debug_print(normal, reduce, "msg->num={}\n", msg->getConstVal());
  theCollective()->global()->reduce<PlusOp<int>, Verify<ReduceOP::Plus>>(
    root, msg.get()
  );
}

TEST_F(TestReduce, test_reduce_max_default_op) {
  auto const my_node = theContext()->getNode();
  auto const root = 0;

  auto msg = makeMessage<SysMsg>(my_node);
  vt_debug_print(normal, reduce, "msg->num={}\n", msg->getConstVal());
  theCollective()->global()->reduce<MaxOp<int>, Verify<ReduceOP::Max>>(
    root, msg.get()
  );
}

TEST_F(TestReduce, test_reduce_min_default_op) {
  auto const my_node = theContext()->getNode();
  auto const root = 0;

  auto msg = makeMessage<SysMsg>(my_node);
  vt_debug_print(normal, reduce, "msg->num={}\n", msg->getConstVal());
  theCollective()->global()->reduce<MinOp<int>, Verify<ReduceOP::Min>>(
    root, msg.get()
  );
}

TEST_F(TestReduce, test_reduce_vec_bool_msg) {

  std::vector<bool> vecOfBool;
  vecOfBool.push_back(false);
  vecOfBool.push_back(true);

  auto const root = 0;
  auto msg = makeMessage<ReduceVecMsg<bool>>(vecOfBool);
  theCollective()->global()->reduce<
    PlusOp<std::vector<bool>>, Verify<ReduceOP::Plus>
  >(root, msg.get());
}

TEST_F(TestReduce, test_reduce_vec_int_msg) {

  std::vector<int> vecOfInt;
  vecOfInt.push_back(0);
  vecOfInt.push_back(1);
  vecOfInt.push_back(2);
  vecOfInt.push_back(3);

  auto const root = 0;
  auto const default_proxy = theObjGroup()->getDefault();
  default_proxy.reduce<
    PlusOp<std::vector<int>>,
    Verify<ReduceOP::Plus>,
    ReduceVecMsg<int>
  >(root, vecOfInt);
}

TEST_F(TestReduce, test_reduce_none_op_cb) {
  called = 0;

  runInEpochCollective([]{
    auto msg = makeMessage<ReduceNoneMsg>();
    auto cb = theCB()->makeSend<ReduceNoneMsg, callbackFn>(0);

    theCollective()->global()->reduce<None>(msg.get(), cb);
  });

  EXPECT_EQ(called, theContext()->getNumNodes());
}

// TEST_F(TestReduce, test_reduce_none_op_lambda_cb) {
//   called = 0;

//   runInEpochCollective([&]{
//     auto msg = makeMessage<ReduceNoneMsg>();
//     auto cb = theCB()->makeSend<ReduceNoneMsg>(
//       [&](ReduceNoneMsg* in_msg) {
//         vt_debug_print(
//           reduce, node,
//           "lambda callback: cur={}: is_root={}, count={}\n",
//           print_ptr(in_msg), print_bool(in_msg->isRoot()), in_msg->getCount()
//         );
//         called += 1;
//       }, 0
//     );

//     theCollective()->global()->reduce<None>(msg.get(), cb);
//   });

//   EXPECT_EQ(called, theContext()->getNumNodes());
// }

}}} // end namespace vt::tests::unit
