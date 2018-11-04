
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

#define DEBUG_SCATTER 0

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::collective;
using namespace vt::tests::unit;

static constexpr std::size_t const num_elms = 4;

struct TestScatter : TestParallelHarness {
  static void scatterHan(int* msg) {
    auto const& this_node = theContext()->getNode();
    int* int_ptr = reinterpret_cast<int*>(msg);
    #if DEBUG_SCATTER
      ::fmt::print(
        "ptr={}, *ptr={}\n", print_ptr(int_ptr), *int_ptr
      );
    #endif
    for (auto i = 0; i < num_elms; i++) {
      #if DEBUG_SCATTER
        ::fmt::print(
          "i={}: this_node={}: val={}, expected={}\n",
          i, this_node, int_ptr[i], this_node * 10 + i
        );
      #endif
      EXPECT_EQ(int_ptr[i], this_node * 10 + i);
    }
  }
};

TEST_F(TestScatter, test_scatter_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& root = 0;

  if (this_node == 0) {
    auto const& elm_size = sizeof(int) * num_elms;
    auto const& total_size = elm_size * num_nodes;
    theCollective()->scatter<int,scatterHan>(
      total_size,elm_size,nullptr,[](NodeType node, void* ptr){
        auto ptr_out = reinterpret_cast<int*>(ptr);
        for (auto i = 0; i < num_elms; i++) {
          *(ptr_out + i) = node * 10 + i;
        }
      }
    );
  }
}


}}} // end namespace vt::tests::unit
