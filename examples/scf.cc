#include "scf.h"

#include <fmt/format.h>

#include <cstdlib>

using namespace ::vt;

static NodeType this_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

struct TestMsg : ::vt::Message {};

static void testHandler(TestMsg* msg) {
  ::fmt::print("{}: testHandler\n", theContext()->getNode());
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  this_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  ::fmt::print("{}: started\n", this_node);

  if (this_node == 0) {
    auto msg = makeSharedMessage<TestMsg>();
    theMsg()->broadcastMsg<TestMsg, testHandler>(msg);
  }

  std::vector<Water> waters(10);
  const auto num_waters = waters.size();

  // All the matrices we will need, trim down later. 
  SparseMartix S(num_waters, num_waters);
  SparseMartix T(num_waters, num_waters);
  SparseMartix V(num_waters, num_waters);
  SparseMartix H(num_waters, num_waters);
  SparseMartix D(num_waters, num_waters);
  SparseMartix F(num_waters, num_waters);

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}

// Until we have integral kernels just fill with dummy data
scf::MatrixBlock scf::two_e_integral(
  Water const& i, Water const& j, Water const& k, Water const& l) {
  return scf::MatrixBlock(i.size() * j.size(), k.size() * l.size());
}

scf::MatrixBlock scf::S_integral(Water const& i, Water const& j) {
  return scf::MatrixBlock(i.size(), j.size());
}

scf::MatrixBlock scf::V_integral(Water const& i, Water const& j) {
  return scf::MatrixBlock(i.size(), j.size());
}

scf::MatrixBlock scf::T_integral(Water const& i, Water const& j) {
  return scf::MatrixBlock(i.size(), j.size());
}
