
#include "transport.h"

using namespace vt;

int main(int argc, char** argv) {
  CollectiveOps::initializeContext(argc, argv);
  CollectiveOps::initializeRuntime();

  auto const& my_node = theContext->getNode();
  auto const& num_nodes = theContext->getNumNodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

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

  while (1) {
    runScheduler();
  }

  return 0;
}
