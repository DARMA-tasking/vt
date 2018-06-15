#include "transport.h"

#include <cstdlib>
#include <eigen3/Eigen/Dense>
#include <vector>

namespace scf {

using IndexType = int32_t;

static constexpr IndexType const nbasis_per_water = 24;
static constexpr IndexType const nshell_per_water = 3;

static constexpr IndexType const i_block = nbasis_per_water;
static constexpr IndexType const j_block = nbasis_per_water;

// struct MatrixBlock : public Eigen::MatrixXd {};
using MatrixBlock = Eigen::MatrixXd;
using Vector3d = Eigen::Vector3d;

//
// TODO if and when we switch to a truly sparse data structure the replication
// and initialization from a dense structure will need to be reimplemented.
//
struct SparseMatrix {
  using BlockType = MatrixBlock;
  using BlockPtrType = MatrixBlock*;

  SparseMatrix(IndexType r, IndexType c)
    : blocks_(r, std::vector<BlockType>(c)), rows_(r), cols_(c) {}

  SparseMatrix(IndexType r, IndexType c, MatrixBlock const& mat)
    : SparseMatrix(r, c) {
    // Check that the input matrix is correctly sized
    assert(mat.rows() == rows_ * nbasis_per_water);
    assert(mat.cols() == cols_ * nbasis_per_water);

    for (auto i = 0; i < rows_; ++i) {
      for (auto j = 0; j < cols_; ++j) {
        if (is_local(i, j)) {
          blocks_[i][j] = mat.block(
            i * nbasis_per_water, j * nbasis_per_water, nbasis_per_water,
            nbasis_per_water);
        }
      }
    }
  }

  // TODO Do this like the above constructor, with the truncate in the loop, if
  // space is an issue.
  SparseMatrix(IndexType r, IndexType c, MatrixBlock const& mat, double thresh)
    : SparseMatrix(r, c, mat) {
    truncate(thresh);
  }

  SparseMatrix(SparseMatrix const&) = default;
  SparseMatrix& operator=(SparseMatrix const&) = default;

  BlockType& block(IndexType i, IndexType j) {
    assert(blocks_.size() >= i && "i must exist");
    assert(blocks_[i].size() >= j && "j must exist");
    return blocks_[i][j];
  }

  // Return True for now until we decide on the distribution
  bool is_local(IndexType i, IndexType j) const { return true; }

  double& globalEl(IndexType i, IndexType j) {
    const auto bi = i / i_block;
    const auto bj = j / j_block;
    const auto ei = i % i_block;
    const auto ej = j % j_block;

    return block(bi, bj)(ei, ej);
  }

  // We need a way to get a copy of the matrix that we can call an Eigensolver
  // on.
  MatrixBlock replicate() const {
    const auto num_waters = blocks_.size();
    const auto total_functions = num_waters * nbasis_per_water;
    MatrixBlock replicated(total_functions, total_functions);
    replicated.setZero();

    for (auto i = 0; i < rows_; ++i) {
      for (auto j = 0; j < cols_; ++j) {
        if (is_local(i, j) && blocks_[i][j].size() > 0) {
          replicated.block(
            i * nbasis_per_water, j * nbasis_per_water, nbasis_per_water,
            nbasis_per_water) = blocks_[i][j];
        }
      }
    }

    // TODO once the Sparse matrix is distributed we will need a way to
    // reduce the replicated matrix since each node only writes the blocks
    // that it owns.

    return replicated;
  }

  // Remove blocks with fnorm smaller than threshold
  void truncate(double thresh) {
    const auto thresh2 = thresh * thresh;
    for (auto& row : blocks_) {
      for (auto& block : row) {
        if (thresh < block.squaredNorm()) {
          block.resize(0, 0);
        }
      }
    }
  }

  protected:
  std::vector<std::vector<BlockType>> blocks_;
  IndexType rows_;
  IndexType cols_;
};


// Later change this to Libint Shell
struct Shell {
  Shell(Vector3d v) : pos_(std::move(v)) {}

  Shell() = default;
  Shell(Shell const&) = default;
  Shell(Shell&&) = default;
  ~Shell() = default;

  Vector3d& center() { return pos_; }
  Vector3d const& center() const { return pos_; }

  private:
  Vector3d pos_;
};

// Water will need to be expanded to have shells and we will select a
// canonical ordering for the shells
struct Water {
  Water(Vector3d O, Vector3d H1, Vector3d H2) : shells_(nshell_per_water) {
    shells_.emplace_back(Shell(std::move(O)));
    shells_.emplace_back(Shell(std::move(H1)));
    shells_.emplace_back(Shell(std::move(H2)));
  }

  Water() = default;

  IndexType nbasis() const { return nbasis_per_water; }

  private:
  std::vector<Shell> shells_;
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

// Compute the Schwarz bounds and the sparse pair list
std::pair<MatrixBlock, std::vector<std::vector<IndexType>>>
Schwarz(std::vector<Water> const&);

} /*end namespace scf*/

int main(int argc, char** argv);
