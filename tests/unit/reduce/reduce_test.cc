
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::reduction;
using namespace vt::tests::unit;

struct MyReduceMsg : ReduceMsg {
  MyReduceMsg(int const& in_num)
    : num(in_num)
  { }

  int num = 0;
};

struct TestReduce : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;

  virtual void SetUp() {
    TestParallelHarness::SetUp();
  }

  static void reducePlus(MyReduceMsg* msg) {
    auto const& this_node = theContext()->getNode();

    printf(
      "cur=%p: is_root=%s, count=%d, next=%p, num=%d\n",
      msg, print_bool(msg->is_root), msg->count, msg->next, msg->num
    );

    if (msg->is_root) {
      printf("final num=%d\n", msg->num);
    } else {
      MyReduceMsg* fst_msg = msg;
      MyReduceMsg* cur_msg = msg->next ? static_cast<MyReduceMsg*>(msg->next) : nullptr;;
      while (cur_msg != nullptr) {
        printf(
          "while fst_msg=%p: cur_msg=%p, is_root=%s, count=%d, next=%p, num=%d\n",
          fst_msg, cur_msg, print_bool(cur_msg->is_root), cur_msg->count, cur_msg->next,
          cur_msg->num
        );

        fst_msg->num += cur_msg->num;
        cur_msg = cur_msg->next ? static_cast<MyReduceMsg*>(cur_msg->next) : nullptr;
      }
    }
  }
};

TEST_F(TestReduce, test_reduce_op) {
  auto const& my_node = theContext()->getNode();
  auto const& root = 0;

  MyReduceMsg* msg = makeSharedMessage<MyReduceMsg>(my_node);
  printf("msg->num=%d\n", msg->num);
  theReduction()->reduce<MyReduceMsg, reducePlus>(root, msg);
}

}}} // end namespace vt::tests::unit
