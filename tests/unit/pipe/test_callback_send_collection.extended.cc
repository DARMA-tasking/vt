/*
//@HEADER
// *****************************************************************************
//
//                  test_callback_send_collection.extended.cc
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
#include "data_message.h"

#include "vt/vrt/collection/manager.h"

#include <memory>

namespace vt { namespace tests { namespace unit { namespace send {

using namespace vt;
using namespace vt::tests::unit;

struct DataMsg : vt::Message {
  DataMsg() = default;
  DataMsg(int in_a, int in_b, int in_c) : a(in_a), b(in_b), c(in_c) { }
  int a = 0, b = 0, c = 0;
};

using TestCallbackSendCollection = TestParallelHarness;

struct TestCol : vt::Collection<TestCol, vt::Index1D> {
  TestCol() = default;
  virtual ~TestCol() = default;

  void check() {
    if (this->getIndex().x() % 2 == 0) {
      EXPECT_EQ(val, 29);
    } else if (this->getIndex().x() % 3 == 0) {
      EXPECT_EQ(val, 15);
    } else if (this->getIndex().x() % 5 == 0) {
      EXPECT_EQ(val, 21);
    } else if (this->getIndex().x() % 7 == 0) {
      EXPECT_EQ(val, 99);
    } else {
      EXPECT_EQ(val, 13);
    }
  }

  void cb1(DataMsg* msg) {
    EXPECT_EQ(msg->a, 8);
    EXPECT_EQ(msg->b, 9);
    EXPECT_EQ(msg->c, 10);
    val = 29;
  }

  void cb2(DataMsg* msg) {
    EXPECT_EQ(msg->a, 8);
    EXPECT_EQ(msg->b, 9);
    EXPECT_EQ(msg->c, 10);
    val = 13;
  }

  void cb4(int a, int b) {
    EXPECT_EQ(a, 8);
    EXPECT_EQ(b, 9);
    val = 15;
  }

  void cb5(std::string str, int a) {
    EXPECT_EQ(a, 8);
    EXPECT_EQ(str, "hello");
    val = 21;
  }

public:
  int32_t val = 17;
};

static void cb3(TestCol* col, DataMsg* msg) {
  EXPECT_EQ(msg->a, 8);
  EXPECT_EQ(msg->b, 9);
  EXPECT_EQ(msg->c, 10);
  col->val = 99;
}

TEST_F(TestCallbackSendCollection, test_callback_send_collection_1) {
  auto const& this_node = theContext()->getNode();

  auto proxy = makeCollection<TestCol>("test_callback_send_collection_1")
    .bulkInsert(Index1D(32))
    .wait();

  runInEpochCollective([&]{
    if (this_node == 0) {
      for (auto i = 0; i < 32; i++) {
        if (i % 2 == 0) {
          auto cb = theCB()->makeSend<&TestCol::cb1>(proxy(i));
          cb.send(8, 9, 10);
        } else if (i % 3 == 0) {
          auto cb = theCB()->makeSend<&TestCol::cb4>(proxy(i));
          cb.send(8, 9);
        } else if (i % 5 == 0) {
          auto cb = theCB()->makeSend<&TestCol::cb5>(proxy(i));
          cb.send("hello", 8);
        } else if (i % 7 == 0) {
          auto cb = theCB()->makeSend<cb3>(proxy(i));
          cb.send(8, 9, 10);
        } else {
          auto cb = theCB()->makeSend<&TestCol::cb2>(proxy(i));
          cb.send(8, 9, 10);
        }
      }
    }
  });

  runInEpochCollective([&]{
    proxy.broadcastCollective<&TestCol::check>();
  });
}

}}}} // end namespace vt::tests::unit::send
