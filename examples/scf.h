#include "transport.h"

#include <cstdlib>
#include <eigen3/Eigen/Dense>
#include <vector>

#include <libint2.hpp>

namespace scf {

using IndexType = int32_t;

static constexpr IndexType const nbasis_per_water = 13;
static constexpr IndexType const nshell_per_water = 9;

static constexpr IndexType const i_block = nbasis_per_water;
static constexpr IndexType const j_block = nbasis_per_water;

using MatrixBlock =
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using Vector3d = Eigen::Vector3d;

//
// TODO if and when we switch to a truly sparse data structure the replication
// and initialization from a dense structure will need to be reimplemented.
//
// Matrix has two states replicated and cyclic
//
struct SparseMatrix {
  using BlockType = MatrixBlock;
  using BlockPtrType = MatrixBlock*;

  SparseMatrix(IndexType r, IndexType c)
    : blocks_(r, std::vector<BlockType>(c)), rows_(r), cols_(c) {
    MPI_Comm_rank(MPI_COMM_WORLD, &owner_);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs_);
  }

  SparseMatrix(IndexType r, IndexType c, bool replicate)
    : blocks_(r, std::vector<BlockType>(c)), rows_(r), cols_(c),
      replicated_(replicate) {
    MPI_Comm_rank(MPI_COMM_WORLD, &owner_);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs_);
  }

  SparseMatrix(
    IndexType r, IndexType c, MatrixBlock const& mat, double thresh,
    bool replicate = false)
    : SparseMatrix(r, c, replicate) {
    // Check that the input matrix is correctly sized
    assert(mat.rows() == rows_ * nbasis_per_water);
    assert(mat.cols() == cols_ * nbasis_per_water);

    const auto thresh2 = thresh * thresh;
    for (auto i = 0; i < rows_; ++i) {
      for (auto j = 0; j < cols_; ++j) {
        if (replicated_ || is_local(i, j)) {
          auto const& block = mat.block(
            i * nbasis_per_water, j * nbasis_per_water, nbasis_per_water,
            nbasis_per_water);
          if (block.squaredNorm() >= thresh2) {
            blocks_[i][j] = block;
          }
        }
      }
    }
  }

  SparseMatrix(SparseMatrix const&) = default;
  SparseMatrix& operator=(SparseMatrix const&) = default;

  BlockType& block(IndexType i, IndexType j) {
    assert(is_local(i, j));
    return blocks_[i][j];
  }

  BlockType const& block(IndexType i, IndexType j) const {
    assert(is_local(i, j));
    return blocks_[i][j];
  }

  // Return True for now until we decide on the distribution
  bool is_local(IndexType i, IndexType j) const {
    return replicated_ ? true : owner(i, j) == owner_;
  }

  // Owner only tells the cyclic owner, if replicated then it's the original
  // owner
  IndexType owner(IndexType i, IndexType j) const {
    return (i * rows_ + j) % nprocs_;
  }

  // Takes a replicated sparse matrix and returns an Eigen::Matrix copy
  MatrixBlock toEig() const {
    assert(replicated_);
    const auto num_waters = rows_;
    const auto total_functions = num_waters * nbasis_per_water;
    MatrixBlock m_eig(total_functions, total_functions);
    m_eig.setZero();

    for (auto i = 0; i < rows_; ++i) {
      for (auto j = 0; j < cols_; ++j) {
        if (blocks_[i][j].size() > 0) {
          m_eig.block(
            i * nbasis_per_water, j * nbasis_per_water, nbasis_per_water,
            nbasis_per_water) = blocks_[i][j];
        }
      }
    }

    return m_eig;
  }

  // Makes the array replicated assumes that no accumulation is necessary
  void replicate() {
    if (replicated_) {
      return;
    }

    // Dumb but foolproof way
    for (auto i = 0; i < rows_; ++i) {
      for (auto j = 0; j < cols_; ++j) {
        bool not_zero = is_local(i, j) ? (blocks_[i][j].size() > 0) : false;

        const auto owns_ij = owner(i, j);

        // Tell everyone if block ij is not zero
        MPI_Bcast(&not_zero, 1, MPI_C_BOOL, owns_ij, MPI_COMM_WORLD);

        if (not_zero) { // Now that we know this block is not zero
          // Resize the block to get the data, if the block is already allocated
          // then resize should be like a no op, making this call safe on the
          // owner node
          auto& block = blocks_[i][j];
          block.resize(nbasis_per_water, nbasis_per_water);
          MPI_Bcast(
            block.data(), block.size(), MPI_DOUBLE, owns_ij, MPI_COMM_WORLD);
        }
      }
    }

    replicated_ = true;
  }

  void accumulate() {
    // For now only on replicated, in the future more general
    assert(replicated_);

    for (auto i = 0; i < rows_; ++i) {
      for (auto j = 0; j < cols_; ++j) {
        auto& block = blocks_[i][j];
        int i_have = (block.size() > 0) ? 1 : 0;
        int anyone_has = 0;
        MPI_Allreduce(
          &i_have, &anyone_has, 1, MPI_INTEGER, MPI_SUM, MPI_COMM_WORLD);

        if (anyone_has > 0) { // Sum up all the contributions
          if (not i_have) {   // I need to allocate space for this
            block.resize(nbasis_per_water, nbasis_per_water);
            block.setZero();
          }
          MPI_Allreduce(
            MPI_IN_PLACE, block.data(), block.size(), MPI_DOUBLE, MPI_SUM,
            MPI_COMM_WORLD);
        }
      }
    }
  }

  // Figure out what percentage of our blocks are dense, result only valid on
  // node 0 for now.
  double fillPercent() const {
    int npresent = 0;
    for (auto i = 0; i < rows_; ++i) {
      for (auto j = 0; j < cols_; ++j) {
        if (is_local(i, j) && blocks_[i][j].size() > 0) {
          ++npresent;
        }
      }
    }
    if (replicated_) {
      return static_cast<double>(npresent) / static_cast<double>(rows_ * cols_);
    } else {
      int global_present = 0;
      MPI_Reduce(
        &npresent, &global_present, 1, MPI_INTEGER, MPI_SUM, 0, MPI_COMM_WORLD);

      return static_cast<double>(global_present) /
        static_cast<double>(rows_ * cols_);
    }
  }

  // Remove blocks with fnorm smaller than threshold
  void truncate(double thresh) {
    const auto thresh2 = thresh * thresh;
    for (auto i = 0; i < rows_; ++i) {
      for (auto j = 0; j < cols_; ++j) {
        if (is_local(i, j)) {
          auto& block = blocks_[i][j];
          if (block.squaredNorm() < thresh2) {
            block.resize(0, 0);
          }
        }
      }
    }
  }

  protected:
  std::vector<std::vector<BlockType>> blocks_;
  IndexType rows_;
  IndexType cols_;
  bool replicated_ = false;
  IndexType owner_;
  IndexType nprocs_;
};


struct Atom {
  Atom(int n, Vector3d pos) : atomic_number(n), r(pos) {}
  Atom() = default;
  int atomic_number;
  Vector3d r;
};

// Water holds all of the shells a water molecule needs
struct Water {

  // Hard code atom ordering O->H1->H2 and then hard code 6-31g basis in
  // NWCHEM order from the basis set exchange.  Also remove general
  // contractions from the SP shells so that we have 9 shells and 13 functions
  // for each water.
  Water(Vector3d O, Vector3d H1, Vector3d H2)
    : atoms_({Atom(8, ang_to_bohr * O), Atom(1, ang_to_bohr * H1),
              Atom(1, ang_to_bohr * H2)}) {
    // Initialize O shells
    // Oxygen core S
    std::vector<double> alpha = {5484.6717000, 825.2349500, 188.0469600,
                                 52.9645000,   16.8975700,  5.7996353};

    std::vector<double> coeffs = {0.0018311, 0.0139501, 0.0684451,
                                  0.2327143, 0.4701930, 0.3585209};

    // Shell 1
    shells_.push_back(
      libint2::Shell{alpha,
                     {{0, false, coeffs}},
                     {{atoms_[0].r[0], atoms_[0].r[1], atoms_[0].r[2]}}});

    // Oxygen SP 1 stuffs
    alpha = {15.5396160, 3.5999336, 1.0137618};
    coeffs = {-0.1107775, -0.1480263, 1.1307670};
    std::vector<double> coeffs2 = {0.0708743, 0.3397528, 0.7271586};

    // Oxygen S1 shell 2
    shells_.push_back(
      libint2::Shell{alpha,
                     {{0, false, coeffs}},
                     {{atoms_[0].r[0], atoms_[0].r[1], atoms_[0].r[2]}}});

    // Oxygen P1 shell 3
    shells_.push_back(
      libint2::Shell{alpha,
                     {{1, false, coeffs2}},
                     {{atoms_[0].r[0], atoms_[0].r[1], atoms_[0].r[2]}}});

    //     // Oxygen S2 shell 4
    shells_.push_back(
      libint2::Shell{{0.2700058},
                     {{0, false, {1.0}}},
                     {{atoms_[0].r[0], atoms_[0].r[1], atoms_[0].r[2]}}});

    // Oxygen P2 shell 5
    shells_.push_back(
      libint2::Shell{{0.2700058},
                     {{1, false, {1.0}}},
                     {{atoms_[0].r[0], atoms_[0].r[1], atoms_[0].r[2]}}});

    // H1 S core shell 6
    shells_.push_back(
      libint2::Shell{{18.7311370, 2.8253937, 0.6401217},
                     {
                       {0, false, {0.03349460, 0.23472695, 0.81375733}},
                     },
                     {{atoms_[1].r[0], atoms_[1].r[1], atoms_[1].r[2]}}});

    // H1 S2 shell 7
    shells_.push_back(
      libint2::Shell{{0.1612778},
                     {
                       {0, false, {1.0}},
                     },
                     {{atoms_[1].r[0], atoms_[1].r[1], atoms_[1].r[2]}}});

    // H2 S core shell 8
    shells_.push_back(
      libint2::Shell{{18.7311370, 2.8253937, 0.6401217},
                     {
                       {0, false, {0.03349460, 0.23472695, 0.81375733}},
                     },
                     {{atoms_[2].r[0], atoms_[2].r[1], atoms_[2].r[2]}}});

    // H1 S2 shell 9
    shells_.push_back(
      libint2::Shell{{0.1612778},
                     {
                       {0, false, {1.0}},
                     },
                     {{atoms_[2].r[0], atoms_[2].r[1], atoms_[2].r[2]}}});
  }

  Water() = default;

  IndexType nbasis() const { return nbasis_per_water; }

  std::array<Atom, 3> const& atoms() const { return atoms_; }
  std::vector<libint2::Shell> const& shells() const { return shells_; }
  IndexType nelectrons() const {
    return atoms_[0].atomic_number + atoms_[1].atomic_number +
      atoms_[2].atomic_number;
  }

  private:
  double ang_to_bohr = 1.889725989;
  std::array<Atom, 3> atoms_;
  std::vector<libint2::Shell> shells_;
}; // namespace scf

// 4 center integrals for 4 different waters
MatrixBlock two_e_integral(
  Water const&, Water const&, Water const&, Water const&, libint2::Engine&);

// Overlap integrals
MatrixBlock one_body_integral(Water const&, Water const&, libint2::Engine&);

// Compute the Schwarz bounds and the sparse pair list
std::pair<MatrixBlock, std::vector<std::vector<IndexType>>>
Schwarz(std::vector<Water> const&, libint2::Engine&);

// Ignore symmetry for now, we will eventually come back and make use of the
// symmetry
void four_center_update(
  SparseMatrix&, SparseMatrix const&, MatrixBlock const&,
  std::vector<std::vector<IndexType>> const&, std::vector<Water> const&, double,
  std::vector<libint2::Engine>&);

MatrixBlock
four_center_update(MatrixBlock const&, Water const&, libint2::Engine&);

std::vector<std::pair<double, std::array<double, 3>>>
nuclei_charges(std::vector<Water> const&);

// Returns the density for a specific water
MatrixBlock matrixBasedScf(Water const& w);

} /*end namespace scf*/

int main(int argc, char** argv);
