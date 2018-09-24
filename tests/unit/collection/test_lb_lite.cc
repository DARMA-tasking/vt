
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
using namespace vt::tests::unit;

struct IterMsg;

struct LBTest : Collection<LBTest,Index1D> {
  LBTest() = default;

  void setValues() {
    val1 = getIndex().x();
    val2 = 100;
    val3 = 384 * val1;
  }

  void assertValues() {
    EXPECT_EQ(val1, getIndex().x());
    EXPECT_EQ(val2, 100);
    EXPECT_EQ(val3, 384 * val1);
  }

  void printValues() {
    ::fmt::print(
      "idx={}, val1={}, val2={}, val3={}\n",
      getIndex().x(), val1, val2, val3
    );
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    Collection<LBTest,Index1D>::serialize(s);
    s | val1 | val2 | val3 | data_2;
  }

  static void iterWork(IterMsg* msg, LBTest* col);

public:
  double data_2 = 1.0;
  int64_t val1 = 0, val2 = 0, val3 = 0;
};

struct IterMsg : CollectionMessage<LBTest> {
  IterMsg() = default;
  explicit IterMsg(int32_t const in_iter) : iter_(in_iter) {}
  int32_t iter_ = 0;
};
using ColProxyType = CollectionIndexProxy<LBTest,Index1D>;
struct IterReduceMsg : collective::ReduceTMsg<NoneType> {
  IterReduceMsg() = default;
  IterReduceMsg(ColProxyType in_proxy, int32_t in_iter)
    : proxy_(in_proxy), cur_iter_(in_iter)
  {}
  ColProxyType proxy_ = {};
  int32_t cur_iter_ = 0;
};
static void startIter(int32_t const iter, ColProxyType proxy);
static TimeType cur_time = 0;
static double weight = 1.0f;
static int32_t num_iter = 8;

struct FinishedIter {
  void operator()(IterReduceMsg* raw_msg) {
    auto msg = promoteMsg(raw_msg);
    auto const new_time = ::vt::timing::Timing::getCurrentTime();
    ::fmt::print(
      "finished iteration: iter={}, num_iter={}, time={}\n",
      msg->cur_iter_,num_iter,new_time-cur_time
    );
    auto const iter = msg->cur_iter_;
    cur_time = new_time;
    if (iter < num_iter) {
      theCollection()->nextPhase<LBTest>(msg->proxy_,iter,[=]{
        startIter(iter+1, msg->proxy_);
      });
    } else {
      msg->proxy_.destroy();
    }
  }
};

/*static*/ void LBTest::iterWork(IterMsg* msg, LBTest* col) {
  double val = 0.1f;
  double val2 = 0.4f;
  auto const idx = col->getIndex().x();
  auto const iter = msg->iter_;
  int64_t const max_work = 1000 * weight;
  int64_t const mid_work = 100 * weight;
  int64_t const min_work = 1 * weight;
  int const x = idx < 8 ? max_work : (idx > 40 ? mid_work : min_work);
  //::fmt::print("proc={}, idx={}, iter={}\n", theContext()->getNode(),idx,iter);
  for (int i = 0; i < 10000 * x; i++) {
    val *= val2 + i*29.4;
    val2 += 1.0;
  }
  col->data_2 += val + val2;
  if (iter == 0) {
    col->setValues();
  } else {
    col->assertValues();
  }
  auto proxy = col->getCollectionProxy();
  auto reduce_msg = makeSharedMessage<IterReduceMsg>(proxy,iter);
  theCollection()->reduceMsg<
    LBTest,
    IterReduceMsg,
    IterReduceMsg::template msgHandler<
      IterReduceMsg, collective::PlusOp<collective::NoneType>, FinishedIter
    >
  >(proxy, reduce_msg);
}

static void startIter(int32_t const iter, ColProxyType proxy) {
  ::fmt::print(
    "startIter: iter={}, cur_iter={}\n", iter, iter
  );
  auto msg = makeSharedMessage<IterMsg>(iter);
  proxy.broadcast<IterMsg,LBTest::iterWork>(msg);
}

struct TestLB : TestParallelHarness { };

TEST_F(TestLB, test_lb_1) {
  auto const& my_node = theContext()->getNode();
  auto const& root = 0;
  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    auto const& range = Index1D(32);
    auto proxy = theCollection()->construct<LBTest>(range);
    cur_time = ::vt::timing::Timing::getCurrentTime();
    startIter(0,proxy);
  }
}

// TEST_F(TestLB, test_lb_multi_1) {
//   auto const& my_node = theContext()->getNode();
//   auto const& root = 0;
//   auto const& this_node = theContext()->getNode();
//   if (this_node == 0) {
//     auto const& range = Index1D(64);
//     auto proxy_1 = theCollection()->construct<LBTest>(range);
//     auto proxy_2 = theCollection()->construct<LBTest>(range);
//     cur_time = ::vt::timing::Timing::getCurrentTime();
//     startIter(0,proxy_1);
//     startIter(0,proxy_2);
//   }
// }

// TEST_F(TestLB, test_lb_multi_2) {
//   auto const& my_node = theContext()->getNode();
//   auto const& root = 0;
//   auto const& this_node = theContext()->getNode();
//   if (this_node == 0) {
//     auto const& range = Index1D(32);
//     auto proxy_1 = theCollection()->construct<LBTest>(range);
//     auto proxy_2 = theCollection()->construct<LBTest>(range);
//     auto proxy_3 = theCollection()->construct<LBTest>(range);
//     cur_time = ::vt::timing::Timing::getCurrentTime();
//     startIter(0,proxy_1);
//     startIter(0,proxy_2);
//     startIter(0,proxy_3);
//   }
// }

}}} // end namespace vt::tests::unit
