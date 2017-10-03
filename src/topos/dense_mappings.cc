
#include "config.h"
#include "context.h"
#include "index.h"
#include "mapping.h"
#include "dense_mappings.h"

#include <cmath>

namespace vt { namespace mapping {

// Default round robin mappings
NodeType dense1DRoundRobinMap(Idx1DRef idx, Idx1DRef max_idx, NodeRef nnodes) {
  return idx.x() % nnodes;
}

NodeType dense2DRoundRobinMap(Idx2DRef idx, Idx2DRef max_idx, NodeRef nnodes) {
  using IndexElmType = typename Index2D::DenseIndexType;
  auto const& lin_idx = linearizeDenseIndex<IndexElmType, 2>(idx, max_idx);
  return lin_idx % nnodes;
}

NodeType dense3DRoundRobinMap(Idx3DRef idx, Idx3DRef max_idx, NodeRef nnodes) {
  using IndexElmType = typename Index3D::DenseIndexType;
  auto const& lin_idx = linearizeDenseIndex<IndexElmType, 3>(idx, max_idx);
  return lin_idx % nnodes;
}

// Default block mappings
NodeType dense1DBlockMap(Idx1DRef idx, Idx1DRef max_idx, NodeRef nnodes) {
  return denseBlockMap<Index1D, 1>(idx, max_idx, nnodes);
}

NodeType dense2DBlockMap(Idx2DRef idx, Idx2DRef max_idx, NodeRef nnodes) {
  return denseBlockMap<Index2D, 2>(idx, max_idx, nnodes);
}

NodeType dense3DBlockMap(Idx3DRef idx, Idx3DRef max_idx, NodeRef nnodes) {
  return denseBlockMap<Index3D, 3>(idx, max_idx, nnodes);
}


}} // end namespace vt::location

