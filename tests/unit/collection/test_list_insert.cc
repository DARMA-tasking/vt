/*
//@HEADER
// *****************************************************************************
//
//                             test_list_insert.cc
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
#include "test_collection_common.h"
#include "data_message.h"

#include "vt/vrt/collection/manager.h"

#include <cstdint>

namespace vt { namespace tests { namespace unit { namespace list_insert {

using namespace vt;
using namespace vt::collective;
using namespace vt::tests::unit;

struct WorkMsg;
struct WorkMsgNDC;

static int32_t num_inserted = 0;
static int32_t num_deleted = 0;
static int32_t num_work = 0;

struct ListInsertTest : Collection<ListInsertTest,Index1D> {
  ListInsertTest() {
    num_inserted++;
  }

  virtual ~ListInsertTest() {
    num_deleted++;
  }

  void work(WorkMsg* msg);
};

void ListInsertTest::work(WorkMsg* msg) {
  vt_print(gen, "num_work={}, idx={}\n", num_work, getIndex());
  num_work++;
}

struct WorkMsg : CollectionMessage<ListInsertTest> {};
using ColProxyType = CollectionIndexProxy<ListInsertTest,Index1D>;

struct NonDefaultConstructibleStruct : Collection<NonDefaultConstructibleStruct,Index1D> {
  using ConstructFnType = vt::vrt::collection::param::ConstructParams<
    NonDefaultConstructibleStruct>::ConstructFnType;

  NonDefaultConstructibleStruct(int) {
    num_inserted++;
  }

  virtual ~NonDefaultConstructibleStruct() {
    num_deleted++;
  }

  void work(WorkMsgNDC* msg);

  static ConstructFnType getConstructor() {
    return [](vt::Index1D) {
      return std::make_unique<NonDefaultConstructibleStruct>(0);
    };
  }
};

struct WorkMsgNDC : CollectionMessage<NonDefaultConstructibleStruct> {};
using ColProxyTypeNDC = CollectionIndexProxy<NonDefaultConstructibleStruct,Index1D>;

void reconstruct(NonDefaultConstructibleStruct*&, void*) {
}

void NonDefaultConstructibleStruct::work(WorkMsgNDC* msg) {
  vt_print(gen, "num_work={}, idx={}\n", num_work, getIndex());
  num_work++;
}

struct TestListInsert : TestParallelHarness { };

static constexpr int32_t const num_elms_per_node = 8;

TEST_F(TestListInsert, test_bounded_list_insert_1) {
  num_inserted = 0;
  num_work = 0;

  auto const num_nodes = theContext()->getNumNodes();

  auto const range = Index1D(num_nodes * num_elms_per_node);
  std::vector<Index1D> list_insert;
  for (int i = 0; i < range.x(); i++) {
    list_insert.emplace_back(Index1D{i});
  }

  auto proxy = vt::makeCollection<ListInsertTest>("test_bounded_list_insert_1")
    .collective(true)
    .bounds(range)
    .listInsert(list_insert)
    .wait();

  EXPECT_EQ(num_inserted, num_elms_per_node);
  num_inserted = 0;

  runInEpochCollective([&]{
    proxy.broadcast<WorkMsg, &ListInsertTest::work>();
  });
  EXPECT_EQ(num_work, num_elms_per_node * num_nodes);
}

TEST_F(TestListInsert, test_bounded_list_insert_no_default_constructor) {
  num_inserted = 0;
  num_work = 0;

  auto const num_nodes = theContext()->getNumNodes();

  auto const range = Index1D(num_nodes * num_elms_per_node);
  std::vector<Index1D> list_insert;
  for (int i = 0; i < range.x(); i++) {
    list_insert.emplace_back(Index1D{i});
  }

  auto proxy = vt::makeCollection<NonDefaultConstructibleStruct>("test_bounded_list_insert_no_default_constructor")
    .collective(true)
    .bounds(range)
    .listInsert(list_insert)
    .elementConstructor(NonDefaultConstructibleStruct::getConstructor())
    .wait();

  EXPECT_EQ(num_inserted, num_elms_per_node);
  num_inserted = 0;

  runInEpochCollective([&]{
    proxy.broadcast<WorkMsgNDC, &NonDefaultConstructibleStruct::work>();
  });
  EXPECT_EQ(num_work, num_elms_per_node * num_nodes);
}

template <typename IndexT>
struct MyMapper : vt::mapping::BaseMapper<IndexT> {
  static vt::ObjGroupProxyType construct() {
    return vt::theObjGroup()->makeCollective<MyMapper<IndexT>>(
      "MyMapper"
    ).getProxy();
  }

  vt::NodeType map(IndexT* idx, int ndim, vt::NodeType num_nodes) override {
    return idx->x() % num_nodes;
  }
};

TEST_F(TestListInsert, test_unbounded_list_insert_2) {
  num_inserted = 0;
  num_work = 0;

  auto const num_nodes = theContext()->getNumNodes();

  auto const range = Index1D(num_nodes * num_elms_per_node);
  std::vector<Index1D> list_insert;
  for (int i = 0; i < range.x(); i++) {
    list_insert.emplace_back(Index1D{i});
  }

  auto proxy = vt::makeCollection<ListInsertTest>("test_unbounded_list_insert_2")
    .collective(true)
    .listInsert(list_insert)
    .template mapperObjGroupConstruct<MyMapper<Index1D>>()
    .wait();

  EXPECT_EQ(num_inserted, num_elms_per_node);
  num_inserted = 0;

  runInEpochCollective([&]{
    proxy.broadcast<WorkMsg, &ListInsertTest::work>();
  });
  EXPECT_EQ(num_work, num_elms_per_node * num_nodes);
}

TEST_F(TestListInsert, test_unbounded_list_insert_no_default_constructor) {
  num_inserted = 0;
  num_work = 0;

  auto const num_nodes = theContext()->getNumNodes();

  auto const range = Index1D(num_nodes * num_elms_per_node);
  std::vector<Index1D> list_insert;
  for (int i = 0; i < range.x(); i++) {
    list_insert.emplace_back(Index1D{i});
  }

  auto proxy = vt::makeCollection<NonDefaultConstructibleStruct>("test_unbounded_list_insert_no_default_constructor")
    .collective(true)
    .listInsert(list_insert)
    .elementConstructor(NonDefaultConstructibleStruct::getConstructor())
    .template mapperObjGroupConstruct<MyMapper<Index1D>>()
    .wait();

  EXPECT_EQ(num_inserted, num_elms_per_node);
  num_inserted = 0;

  runInEpochCollective([&]{
    proxy.broadcast<WorkMsgNDC, &NonDefaultConstructibleStruct::work>();
  });
  EXPECT_EQ(num_work, num_elms_per_node * num_nodes);
}

TEST_F(TestListInsert, test_bounded_list_insert_here_3) {
  num_inserted = 0;
  num_work = 0;

  auto const num_nodes = theContext()->getNumNodes();
  auto const this_node = theContext()->getNode();
  auto const range = Index1D(num_nodes * num_elms_per_node);

  std::vector<std::tuple<vt::Index1D, std::unique_ptr<ListInsertTest>>> elms;

  for (int i = 0; i < range.x(); i++) {
    if (i % num_nodes == this_node) {
      Index1D ix{i};
      elms.emplace_back(
        std::make_tuple(ix, std::make_unique<ListInsertTest>())
      );
    }
  }

  auto proxy = vt::makeCollection<ListInsertTest>("test_bounded_list_insert_here_3")
    .collective(true)
    .bounds(range)
    .listInsertHere(std::move(elms))
    .wait();

  EXPECT_EQ(num_inserted, num_elms_per_node);
  num_inserted = 0;

  runInEpochCollective([&]{
    proxy.broadcast<WorkMsg, &ListInsertTest::work>();
  });
  EXPECT_EQ(num_work, num_elms_per_node * num_nodes);
}

TEST_F(TestListInsert, test_bounded_list_insert_here_no_default_constructor) {
  num_inserted = 0;
  num_work = 0;

  auto const num_nodes = theContext()->getNumNodes();
  auto const this_node = theContext()->getNode();
  auto const range = Index1D(num_nodes * num_elms_per_node);

  std::vector<std::tuple<vt::Index1D, std::unique_ptr<NonDefaultConstructibleStruct>>> elms;
  for (int i = 0; i < range.x(); i++) {
    if (i % num_nodes == this_node) {
      Index1D ix{i};
      elms.emplace_back(
        std::make_tuple(ix, std::make_unique<NonDefaultConstructibleStruct>(0))
      );
    }
  }

  auto proxy = vt::makeCollection<NonDefaultConstructibleStruct>("test_bounded_list_insert_here_no_default_constructor")
    .collective(true)
    .bounds(range)
    .listInsertHere(std::move(elms))
    .wait();

  EXPECT_EQ(num_inserted, num_elms_per_node);
  num_inserted = 0;

  runInEpochCollective([&]{
    proxy.broadcast<WorkMsgNDC, &NonDefaultConstructibleStruct::work>();
  });
  EXPECT_EQ(num_work, num_elms_per_node * num_nodes);
}

TEST_F(TestListInsert, test_unbounded_list_insert_here_4) {
  num_inserted = 0;
  num_work = 0;

  auto const num_nodes = theContext()->getNumNodes();
  auto const this_node = theContext()->getNode();
  auto const range = Index1D(num_nodes * num_elms_per_node);

  std::vector<std::tuple<vt::Index1D, std::unique_ptr<ListInsertTest>>> elms;

  for (int i = 0; i < range.x(); i++) {
    if (i % num_nodes == this_node) {
      Index1D ix{i};
      elms.emplace_back(
        std::make_tuple(ix, std::make_unique<ListInsertTest>())
      );
    }
  }

  auto proxy = vt::makeCollection<ListInsertTest>("test_unbounded_list_insert_here_4")
    .collective(true)
    .listInsertHere(std::move(elms))
    .template mapperObjGroupConstruct<MyMapper<Index1D>>()
    .wait();

  EXPECT_EQ(num_inserted, num_elms_per_node);
  num_inserted = 0;

  runInEpochCollective([&]{
    proxy.broadcast<WorkMsg, &ListInsertTest::work>();
  });
  EXPECT_EQ(num_work, num_elms_per_node * num_nodes);
}

TEST_F(TestListInsert, test_unbounded_list_insert_here_no_default_constructor) {
  num_inserted = 0;
  num_work = 0;

  auto const num_nodes = theContext()->getNumNodes();
  auto const this_node = theContext()->getNode();
  auto const range = Index1D(num_nodes * num_elms_per_node);

  std::vector<std::tuple<vt::Index1D, std::unique_ptr<NonDefaultConstructibleStruct>>> elms;
  for (int i = 0; i < range.x(); i++) {
    if (i % num_nodes == this_node) {
      Index1D ix{i};
      elms.emplace_back(
        std::make_tuple(ix, std::make_unique<NonDefaultConstructibleStruct>(0))
      );
    }
  }

  auto proxy = vt::makeCollection<NonDefaultConstructibleStruct>("test_unbounded_list_insert_here_no_default_constructor")
    .collective(true)
    .listInsertHere(std::move(elms))
    .template mapperObjGroupConstruct<MyMapper<Index1D>>()
    .wait();

  EXPECT_EQ(num_inserted, num_elms_per_node);
  num_inserted = 0;

  runInEpochCollective([&]{
    proxy.broadcast<WorkMsgNDC, &NonDefaultConstructibleStruct::work>();
  });
  EXPECT_EQ(num_work, num_elms_per_node * num_nodes);
}

TEST_F(TestListInsert, test_unbounded_list_insert_here_no_default_constructor_empty_rank) {
  num_inserted = 0;
  num_work = 0;

  auto const num_nodes = theContext()->getNumNodes();
  auto const this_node = theContext()->getNode();
  auto const range = Index1D((num_nodes - 1) * num_elms_per_node);

  std::vector<std::tuple<vt::Index1D, std::unique_ptr<NonDefaultConstructibleStruct>>> elms;
  for (int i = 0; i < range.x(); i++) {
    if (i % (num_nodes - 1) == this_node) {
      Index1D ix{i};
      elms.emplace_back(
        std::make_tuple(ix, std::make_unique<NonDefaultConstructibleStruct>(0))
      );
    }
  }

  auto proxy = vt::makeCollection<NonDefaultConstructibleStruct>(
    "test_unbounded_list_insert_here_no_default_constructor_empty_rank"
  )
    .collective(true)
    .listInsertHere(std::move(elms))
    .template mapperObjGroupConstruct<MyMapper<Index1D>>()
    .wait();

  if (this_node < num_nodes - 1) {
    EXPECT_EQ(num_inserted, num_elms_per_node);
  } else {
    EXPECT_EQ(num_inserted, 0);
  }
  num_inserted = 0;

  runInEpochCollective([&]{
    proxy.broadcast<WorkMsgNDC, &NonDefaultConstructibleStruct::work>();
  });
  if (this_node < num_nodes - 1) {
    // all ranks broadcast to all other ranks
    EXPECT_EQ(num_work, num_elms_per_node * num_nodes);
  } else {
    // but there is nothing on the final rank to do work
    EXPECT_EQ(num_work, 0);
  }
}

TEST_F(TestListInsert, test_bounded_bulk_insert_no_default_constructor) {
  num_inserted = 0;
  num_work = 0;

  auto const num_nodes = theContext()->getNumNodes();
  auto const range = Index1D(num_nodes * num_elms_per_node);

  auto proxy = vt::makeCollection<NonDefaultConstructibleStruct>("test_bounded_bulk_insert_no_default_constructor")
    .collective(true)
    .bounds(range)
    .bulkInsert()
    .elementConstructor(NonDefaultConstructibleStruct::getConstructor())
    .template mapperObjGroupConstruct<MyMapper<Index1D>>()
    .wait();

  EXPECT_EQ(num_inserted, num_elms_per_node);
  num_inserted = 0;

  runInEpochCollective([&]{
    proxy.broadcast<WorkMsgNDC, &NonDefaultConstructibleStruct::work>();
  });
  EXPECT_EQ(num_work, num_elms_per_node * num_nodes);
}

TEST_F(TestListInsert, test_bounded_mix_list_insert_no_default_constructor) {
  num_inserted = 0;
  num_work = 0;

  auto const num_nodes = theContext()->getNumNodes();
  auto const this_node = theContext()->getNode();
  auto const range = Index1D(num_nodes * num_elms_per_node);

  std::vector<Index1D> list_insert;
  for (int i = 0; i < range.x() / 2; i++) {
    list_insert.emplace_back(Index1D{i});
  }

  std::vector<std::tuple<vt::Index1D, std::unique_ptr<NonDefaultConstructibleStruct>>> elms;
  for (int i = range.x() / 2; i < range.x(); i++) {
    if (i % num_nodes == this_node) {
      Index1D ix{i};
      elms.emplace_back(
        std::make_tuple(ix, std::make_unique<NonDefaultConstructibleStruct>(0))
      );
    }
  }

  auto proxy = vt::makeCollection<NonDefaultConstructibleStruct>("test_bounded_mix_list_insert_no_default_constructor")
    .collective(true)
    .bounds(range)
    .listInsert(list_insert)
    .listInsertHere(std::move(elms))
    .elementConstructor(NonDefaultConstructibleStruct::getConstructor())
    .template mapperObjGroupConstruct<MyMapper<Index1D>>()
    .wait();

  EXPECT_EQ(num_inserted, num_elms_per_node);
  num_inserted = 0;

  runInEpochCollective([&]{
    proxy.broadcast<WorkMsgNDC, &NonDefaultConstructibleStruct::work>();
  });
  EXPECT_EQ(num_work, num_elms_per_node * num_nodes);
}

}}}} // end namespace vt::tests::unit::list_insert
