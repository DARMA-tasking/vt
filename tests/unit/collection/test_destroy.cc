
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "test_collection_common.h"
#include "data_message.h"

#include "transport.h"

#include <cstdint>

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::collective;
using namespace vt::index;
using namespace vt::mapping;
using namespace vt::vrt;
using namespace vt::vrt::collection;
using namespace vt::tests::unit;

struct WorkMsg;

static int32_t num_destroyed = 0;

struct DestroyTest : Collection<DestroyTest,Index1D> {
  DestroyTest() = default;

  virtual ~DestroyTest() {
    /// ::fmt::print("destroying collection: idx={}\n", getIndex().x());
    num_destroyed++;
  }

  static void work(WorkMsg* msg, DestroyTest* col);
};

struct WorkMsg : CollectionMessage<DestroyTest> {};
using ColProxyType = CollectionIndexProxy<DestroyTest,Index1D>;
struct CollReduceMsg : collective::ReduceTMsg<NoneType> {
  CollReduceMsg() = default;
  explicit CollReduceMsg(ColProxyType in_proxy)
    : proxy_(in_proxy)
  {}
  ColProxyType proxy_ = {};
};

struct FinishedWork {
  void operator()(CollReduceMsg* msg) {
    msg->proxy_.destroy();
  }
};

/*static*/ void DestroyTest::work(WorkMsg* msg, DestroyTest* col) {
  auto proxy = col->getCollectionProxy();
  auto reduce_msg = makeSharedMessage<CollReduceMsg>(proxy);
  theCollection()->reduceMsg<
    DestroyTest,
    CollReduceMsg,
    CollReduceMsg::template msgHandler<
      CollReduceMsg, collective::PlusOp<collective::NoneType>, FinishedWork
    >
  >(proxy,reduce_msg);
}

struct TestDestroy : TestParallelHarness { };

static constexpr int32_t const num_elms_per_node = 8;

TEST_F(TestDestroy, test_destroy_1) {
  auto const& root = 0;
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  if (this_node == 0) {
    auto const& range = Index1D(num_nodes * num_elms_per_node);
    auto proxy = theCollection()->construct<DestroyTest>(range);
    auto msg = makeSharedMessage<WorkMsg>();
    proxy.broadcast<WorkMsg,DestroyTest::work>(msg);
  }
  theTerm()->attachGlobalTermAction([]{
    /// ::fmt::print("num destroyed={}\n", num_destroyed);
    // Relies on default mapping equally distributing
    EXPECT_EQ(num_destroyed, num_elms_per_node);
  });
}

}}} // end namespace vt::tests::unit
