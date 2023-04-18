/*
//@HEADER
// *****************************************************************************
//
//                  test_callback_bcast_collection.extended.cc
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/vrt/collection/manager.h"

#include <memory>

namespace vt { namespace tests { namespace unit { namespace bcast {

using namespace vt;
using namespace vt::tests::unit;

struct CallbackMsg : vt::Message {
  CallbackMsg() = default;
  explicit CallbackMsg(Callback<> in_cb) : cb_(in_cb) { }

  Callback<> cb_;
};

struct DataMsg : vt::Message {
  DataMsg() = default;
  DataMsg(int in_a, int in_b, int in_c) : a(in_a), b(in_b), c(in_c) { }
  int a = 0, b = 0, c = 0;
};

struct CallbackDataMsg : vt::Message {
  CallbackDataMsg() = default;
  explicit CallbackDataMsg(Callback<DataMsg> in_cb) : cb_(in_cb) { }

  Callback<DataMsg> cb_;
};

struct TestCallbackBcastCollection : TestParallelHarness {
  static void testHandler(CallbackDataMsg* msg) {
    msg->cb_.send(8,9,10);
  }
  static void testHandlerEmpty(CallbackMsg* msg) {
    msg->cb_.send();
  }
};

struct TestCol : vt::Collection<TestCol, vt::Index1D> {
  TestCol() = default;

  virtual ~TestCol() = default;

  void check() {
    if (which_cb == 1) {
      EXPECT_EQ(val, 29);
    } else if (which_cb == 2) {
      EXPECT_EQ(val, 13);
    } else if (which_cb == 3) {
      EXPECT_EQ(val, 15);
    } else if (which_cb == 4) {
      EXPECT_EQ(val, 21);
    } else if (which_cb == 5) {
      EXPECT_EQ(val, 99);
    } else {
      vtAbort("Should be unreachable");
    }
  }

  void cb1(DataMsg* msg) {
    EXPECT_EQ(msg->a, 8);
    EXPECT_EQ(msg->b, 9);
    EXPECT_EQ(msg->c, 10);
    val = 29;
    which_cb = 1;
  }

  void cb2(DataMsg* msg) {
    EXPECT_EQ(msg->a, 8);
    EXPECT_EQ(msg->b, 9);
    EXPECT_EQ(msg->c, 10);
    val = 13;
    which_cb = 2;
  }

  void cb3(int a, int b) {
    EXPECT_EQ(a, 8);
    EXPECT_EQ(b, 9);
    val = 15;
    which_cb = 3;
  }

  void cb4(std::string str, int a) {
    EXPECT_EQ(a, 8);
    EXPECT_EQ(str, "hello");
    val = 21;
    which_cb = 4;
  }

public:
  int32_t val = 17;
  int which_cb = 0;
};

static void cb5(TestCol* col, DataMsg* msg) {
  EXPECT_EQ(msg->a, 8);
  EXPECT_EQ(msg->b, 9);
  EXPECT_EQ(msg->c, 10);
  col->val = 99;
  col->which_cb = 5;
}

TEST_F(TestCallbackBcastCollection, test_callback_bcast_collection_1) {
  auto const& this_node = theContext()->getNode();
  auto const& range = Index1D(32);

  vt::CollectionProxy<TestCol, vt::Index1D> proxy;

  if (this_node == 0) {
    proxy = theCollection()->construct<TestCol>(
      range, "test_callback_bcast_collection_1"
    );
  }

  runInEpochCollective([&]{
    if (this_node == 0) {
      auto cb = theCB()->makeBcast<&TestCol::cb1>(proxy);
      cb.send(8,9,10);
    }
  });

  runInEpochCollective([&]{
    if (this_node == 0) {
      proxy.broadcast<&TestCol::check>();
    }
  });
}

TEST_F(TestCallbackBcastCollection, test_callback_bcast_collection_2) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes < 2) {
    return;
  }

  auto const& range = Index1D(32);

  vt::CollectionProxy<TestCol, vt::Index1D> proxy;

  if (this_node == 0) {
    proxy = theCollection()->construct<TestCol>(
      range, "test_callback_bcast_collection_2"
    );
  }

  runInEpochCollective([&]{
    if (this_node == 0) {
      auto next = this_node + 1 < num_nodes ? this_node + 1 : 0;
      auto cb = theCB()->makeBcast<&TestCol::cb2>(proxy);
      auto msg = makeMessage<CallbackDataMsg>(cb);
      theMsg()->sendMsg<testHandler>(next, msg);
    }
  });

  runInEpochCollective([&]{
    if (this_node == 0) {
      proxy.broadcast<&TestCol::check>();
    }
  });
}

TEST_F(TestCallbackBcastCollection, test_callback_bcast_collection_param_1) {
  auto const this_node = theContext()->getNode();

  auto proxy = makeCollection<TestCol>("test_callback_bcast_collection_param_1")
    .bulkInsert(Index1D(32))
    .wait();

  runInEpochCollective([&]{
    if (this_node == 0) {
      auto cb = theCB()->makeBcast<&TestCol::cb3>(proxy);
      cb.send(8, 9);
    }
  });

  runInEpochCollective([&]{
    proxy.broadcastCollective<&TestCol::check>();
  });
}

TEST_F(TestCallbackBcastCollection, test_callback_bcast_collection_param_2) {
  auto const this_node = theContext()->getNode();

  auto proxy = makeCollection<TestCol>("test_callback_bcast_collection_param_2")
    .bulkInsert(Index1D(32))
    .wait();

  runInEpochCollective([&]{
    if (this_node == 0) {
      auto cb = theCB()->makeBcast<&TestCol::cb4>(proxy);
      cb.send("hello", 8);
    }
  });

  runInEpochCollective([&]{
    proxy.broadcastCollective<&TestCol::check>();
  });
}

TEST_F(TestCallbackBcastCollection, test_callback_bcast_collection_3) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes < 2) {
    return;
  }

  auto const& range = Index1D(32);

  vt::CollectionProxy<TestCol, vt::Index1D> proxy;

  if (this_node == 0) {
    proxy = theCollection()->construct<TestCol>(
      range, "test_callback_bcast_collection_3"
    );
  }

  runInEpochCollective([&]{
    if (this_node == 0) {
      auto next = this_node + 1 < num_nodes ? this_node + 1 : 0;
      auto cb = theCB()->makeBcast<cb5>(proxy);
      auto msg = makeMessage<CallbackDataMsg>(cb);
      theMsg()->sendMsg<testHandler>(next, msg);
    }
  });

  runInEpochCollective([&]{
    if (this_node == 0) {
      proxy.broadcast<&TestCol::check>();
    }
  });
}

}}}} // end namespace vt::tests::unit::bcast
