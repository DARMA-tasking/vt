
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_harness.h"

#include "transport.h"
#include "index.h"
#include "mapping.h"

#include <vector>

namespace vt { namespace tests { namespace unit {

struct TestMapping : TestHarness { };

TEST_F(TestMapping, test_mapping_block_1d) {
  using namespace vt;

  static constexpr index::Index1D::DenseIndexType const val = 16;
  static constexpr index::Index1D::DenseIndexType const max = 256;
  static constexpr NodeType const nnodes = 8;

  index::Index1D idx(val);
  index::Index1D max_idx(max);

  EXPECT_EQ(idx[0], val);
  EXPECT_EQ(max_idx[0], max);

  std::vector<index::Index1D::IndexSizeType> map_cnt(nnodes);

  EXPECT_EQ(max % nnodes, 0);

  for (int i = 0; i < max; i++) {
    index::Index1D idx_test(i);
    auto const& node = mapping::dense1DBlockMap(&idx_test, &max_idx, nnodes);
    //printf("node=%d,idx val=%d, idx max=%d\n", node, idx_test[0], max_idx[0]);
    map_cnt[node]++;
  }

  EXPECT_EQ(map_cnt.size(), nnodes);

  index::Index1D::IndexSizeType map_count_fst = map_cnt[0];

  for (int i = 1; i < nnodes; i++) {
    EXPECT_EQ(map_cnt[i], map_count_fst);
  }
}

TEST_F(TestMapping, test_mapping_block_2d) {
  using namespace vt;

  static constexpr index::Index1D::DenseIndexType const max0 = 16;
  static constexpr index::Index1D::DenseIndexType const max1 = 16;
  static constexpr NodeType const nnodes = 8;

  index::Index2D max_idx(max0, max1);

  EXPECT_EQ(max_idx[0], max0);
  EXPECT_EQ(max_idx[1], max1);

  std::vector<index::Index1D::IndexSizeType> map_cnt(nnodes);

  EXPECT_EQ((max0 * max1) % nnodes, 0);

  for (int i = 0; i < max_idx[0]; i++) {
    for (int j = 0; j < max_idx[1]; j++) {
      index::Index2D idx_test(i, j);
      auto const& node = mapping::dense2DBlockMap(&idx_test, &max_idx, nnodes);
      printf(
        "node=%d,idx val=[%d,%d], idx max=[%d,%d]\n",
        node, idx_test[0], idx_test[1], max_idx[0], max_idx[1]
      );
      map_cnt[node]++;
    }
  }

  EXPECT_EQ(map_cnt.size(), nnodes);

  index::Index2D::IndexSizeType map_count_fst = map_cnt[0];

  for (int i = 1; i < nnodes; i++) {
    EXPECT_EQ(map_cnt[i], map_count_fst);
  }
}

}}} // end namespace vt::tests::unit
