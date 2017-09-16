
#include <cassert>

#include "transport.h"

using namespace vt;

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext->getNode();
  auto const& num_nodes = theContext->getNumNodes();

  index::Index2D idx(2, 3);
  index::Index2D idx2(5, 10);
  index::Index2D idx3 = idx2 - idx;

  index::Index1D idx_1d(10);
  index::Index1D idx_1d_2(20);

  auto node = mapping::dense1DBlockMap(idx_1d, idx_1d_2, num_nodes);

  printf(
    "idx=%s, idx2=%s, idx3=%s, size=%ld, node=%d\n",
    idx.toString().c_str(), idx2.toString().c_str(), idx3.toString().c_str(),
    sizeof(idx), node
  );

  int const dim1 = 4, dim2 = 5;
  index::Index2D idx_a(2, 2);
  index::Index2D idx_a_max(dim1, dim2);

  auto node_a = mapping::dense2DBlockMap(idx_a, idx_a_max, num_nodes);

  int cur_val = 0;
  for (int i = 0; i < dim1; i++) {
    for (int j = 0; j < dim2; j++) {
      auto cur_idx = index::Index2D(i,j);
      auto lin_idx = mapping::linearizeDenseIndex(cur_idx, idx_a_max);
      printf(
        "idx=%s, max=%s, lin=%d\n",
        cur_idx.toString().c_str(), idx_a_max.toString().c_str(), lin_idx
      );
      assert(lin_idx == cur_val);
      cur_val++;
    }
  }

  auto const& idx_a_str = idx_a.toString().c_str();
  auto const& idx_a_max_str = idx_a_max.toString().c_str();

  printf(
    "idx_a=%s, indx_a_max=%s, node=%d\n", idx_a_str, idx_a_max_str, node_a
  );

  while (vtIsWorking) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
