
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

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | val1 | val2 | val3 | data_2;
  }

  static void iterWork(IterMsg* msg, LBTest* col);

public:
  double data_2 = 1.0;
  int64_t val1 = 0, val2 = 0, val3 = 0;
};

struct IterMsg : CollectionMessage<LBTest> {
  IterMsg() = default;
  explicit IterMsg(int64_t const in_iter) : iter_(in_iter) {}
  int64_t iter_ = 0;
};
struct IterReduceMsg : collective::ReduceTMsg<NoneType> {};
static void startIter(int32_t const iter);
static int32_t cur_iter = 0;
static TimeType cur_time = 0;
static double weight = 1.0f;
static int32_t num_iter = 8;
static CollectionIndexProxy<LBTest,Index1D> proxy = {};

struct FinishedIter {
  void operator()(IterReduceMsg* msg) {
    auto const new_time = ::vt::timing::Timing::getCurrentTime();
    ::fmt::print(
      "finished iteration: iter={}, num_iter={}, time={}\n",
      cur_iter,num_iter,new_time-cur_time
    );
    cur_iter++;
    cur_time = new_time;
    if (cur_iter < num_iter) {
      theCollection()->nextPhase<LBTest>(proxy,cur_iter-1,[=]{
        startIter(cur_iter);
      });
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
  for (int i = 0; i < 10000 * x; i++) {
    val *= val2 + i*29.4;
    val2 += 1.0;
  }
  col->data_2 += val + val2;
  auto reduce_msg = makeSharedMessage<IterReduceMsg>();
  theCollection()->reduceMsg<
    LBTest,
    IterReduceMsg,
    IterReduceMsg::template msgHandler<
      IterReduceMsg, collective::PlusOp<collective::NoneType>, FinishedIter
    >
  >(col->getCollectionProxy(), reduce_msg);
}

static void startIter(int32_t const iter) {
  ::fmt::print(
    "startIter: iter={}, cur_iter={}\n", iter, cur_iter
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
    proxy = theCollection()->construct<LBTest>(range);
    cur_time = ::vt::timing::Timing::getCurrentTime();
    startIter(0);
  }

}

}}} // end namespace vt::tests::unit
