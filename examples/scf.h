#include "transport.h"

#include <cstdlib>
#include <eigen3/Eigen/Dense>
#include <vector>

namespace scf {

using IndexType = int32_t;

static constexpr IndexType const i_block = 24;
static constexpr IndexType const j_block = 24;
static constexpr IndexType const nbasis_per_water = 24;

// struct MatrixBlock : public Eigen::MatrixXd {};
using MatrixBlock = Eigen::MatrixXd;
using Vector3d = Eigen::Vector3d;

struct SparseMartix {
  using BlockType = MatrixBlock;
  using BlockPtrType = MatrixBlock*;

  SparseMartix(IndexType r, IndexType c)
    : blocks_(r, std::vector<BlockType>(c)), rows_(r), cols_(c) {}

  BlockType& block(IndexType i, IndexType j) {
    assert(blocks_.size() >= i && "i must exist");
    assert(blocks_[i].size() >= j && "j must exist");
    return blocks_[i][j];
  }

  double& globalEl(IndexType i, IndexType j) {
    const auto bi = i / i_block;
    const auto bj = j / j_block;
    const auto ei = i % i_block;
    const auto ej = j % j_block;

    return block(bi, bj)(ei, ej);
  }

  // We need a way to get a copy of the matrix that we can call an Eigensolver
  // on.
  MatrixBlock replicate() const {}

  protected:
  std::vector<std::vector<BlockType>> blocks_;
  IndexType rows_;
  IndexType cols_;
};

// Assume we hard coded the basis.
static constexpr IndexType const nfunc_per_water = 24;

// Water will need to be expanded to have shells and we will select a canonical
// ordering for the shells
struct Water {
  Water(Vector3d v) : pos_(std::move(v)) {}

  IndexType size() const { return nfunc_per_water; }

  private:
  Vector3d pos_;
};

// 4 center integrals for 4 different waters
MatrixBlock
two_e_integral(Water const&, Water const&, Water const&, Water const&);

// Overlap integrals
MatrixBlock S_integral(Water const&, Water const&);

// Nuclear attraction integrals
MatrixBlock V_integral(Water const&, Water const&);

// Kinetic energy integrals
MatrixBlock T_integral(Water const&, Water const&);

} /*end namespace scf*/

int main(int argc, char** argv);
