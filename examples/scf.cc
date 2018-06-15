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

  std::vector<scf::Water> waters(10);
  const auto num_waters = waters.size();

  // All the matrices we will need, trim down later.
  scf::SparseMatrix S(num_waters, num_waters);
  scf::SparseMatrix H(num_waters, num_waters);
  scf::SparseMatrix F(num_waters, num_waters);

  // Populate the matrices with information
  for (auto i = 0; i < num_waters; ++i) {
    auto const& wi = waters[i];
    for (auto j = 0; j < num_waters; ++j) {
      if (S.is_local(i, j)) { // Assume that H has the same distribution as S
        auto const& wj = waters[j];
        S.block(i, j) = S_integral(wi, wj);
        H.block(i, j) = V_integral(wi, wj) + T_integral(wi, wj);
      }
    }
  }

  auto truncate_thresh = 1e-8;
  S.truncate(truncate_thresh);
  H.truncate(truncate_thresh);

  // Initialize guess assuming that D = 0;
  F = H;

  // Bootstrap SCF
  auto Srep = S.replicate();
  auto Frep = F.replicate();

  const auto water_occupation = 5;
  const auto nocc = water_occupation * num_waters;

  auto make_new_density = [num_waters, nocc](auto F, auto S, double thresh) {
    // TODO Compute on one node and bcast, luckily Eigen will work this way
    // since there are no phase issues
    Eigen::GeneralizedSelfAdjointEigenSolver<scf::MatrixBlock> gevd(F, S);
    scf::MatrixBlock const& C = gevd.eigenvectors().leftCols(nocc);
    scf::MatrixBlock Drep = C * C.transpose();

    return scf::SparseMatrix(num_waters, num_waters, Drep, thresh);
  };

  auto D = make_new_density(Frep, Srep, truncate_thresh);
  scf::MatrixBlock Q;
  std::vector<std::vector<scf::IndexType>> sparse_pair_list;
  std::tie(Q, sparse_pair_list) = scf::Schwarz(waters);

  auto iter = 0;
  while (3 != iter++) {
    ::fmt::print("Starting iteration {}\n", iter);

    // TODO Make a new fock matrix given the density
    // four_center_update(F, D, waters);

    D = make_new_density(Frep, Srep, truncate_thresh);
  }


  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}

// Until we have integral kernels just fill with dummy data
scf::MatrixBlock scf::two_e_integral(
  Water const& i, Water const& j, Water const& k, Water const& l) {
  return scf::MatrixBlock::Random(
    i.nbasis() * j.nbasis(), k.nbasis() * l.nbasis());
}

scf::MatrixBlock scf::S_integral(Water const& i, Water const& j) {
  return scf::MatrixBlock::Random(i.nbasis(), j.nbasis());
}

scf::MatrixBlock scf::V_integral(Water const& i, Water const& j) {
  return scf::MatrixBlock::Random(i.nbasis(), j.nbasis());
}

scf::MatrixBlock scf::T_integral(Water const& i, Water const& j) {
  return scf::MatrixBlock::Random(i.nbasis(), j.nbasis());
}

std::pair<scf::MatrixBlock, std::vector<std::vector<scf::IndexType>>>
scf::Schwarz(std::vector<Water> const& waters) {
  const auto num_waters = waters.size();
  // To start off let's just do screening on individual waters, we can make it
  // do shell level screening later.
  scf::MatrixBlock Q(num_waters, num_waters);
  Q.setZero();

  std::vector<std::vector<scf::IndexType>> sparse_pair_list(num_waters);

  // TODO should distribute and reduce the matrix and the pair list
  for (auto i = 0; i < num_waters; ++i) {
    auto const& wi = waters[i];

    for (auto j = 0; j < num_waters; ++j) {
      auto const& wj = waters[j];

      Q(i, j) = scf::two_e_integral(wi, wj, wi, wj).norm();

      if (Q(i, j) > 1e-12) { // Screen the sparse pairs
        sparse_pair_list[i].push_back(j);
      }
    }
  }

  return std::make_pair(std::move(Q), std::move(sparse_pair_list));
}
