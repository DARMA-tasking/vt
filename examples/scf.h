
#include "transport.h"

#include <cstdlib>
#include <vector>

namespace scf {

using IndexType = int32_t;

static constexpr IndexType const i_block = 32;
static constexpr IndexType const j_block = 32;

struct MatrixBlock : public eigen::MatrixXd { };

struct SparseMartix {
  using BlockType = MatrixBlock;
  using BlockPtrType = MatrixBlock*;

  BlockPtrType& getBlock(IndexType i, IndexType j) {
    assert(blocks_.size() >= i && "i must exist");
    assert(blocks_[i].size() >= j && "j must exist");
    return blocks_[i][j];
  }

  BlockPtrType getBlockGlobal(IndexType i, IndexType j) {
    auto bi = i / i_block;
    auto bj = j / j_block;
    auto ei = i % i_block;
    auto ej = j % j_block;
  }

protected:
  std::vector<std::vector<BlockType>> blocks_;
};

} /*end namespace scf*/

int main(int argc, char** argv);
