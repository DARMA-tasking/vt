
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

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

struct TestReduce : TestParallelHarness {
  using TestMsg = TestStaticBytesShortMsg<4>;

  virtual void SetUp() {
    TestParallelHarness::SetUp();
  }

  static void reducePlus(MyReduceMsg* msg) {
    fmt::print(
      "cur={}: is_root={}, count={}, next={}, num={}\n",
      print_ptr(msg), print_bool(msg->is_root), msg->count,
      print_ptr(msg->next), msg->num
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

}}} // end namespace vt::tests::unit
