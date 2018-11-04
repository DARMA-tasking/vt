
#include "vt/config.h"
#include "vt/topos/mapping/dense/dense.h"
#include "vt/topos/index/index.h"

namespace vt { namespace mapping {

NodeType defaultDenseIndex1DMap(Idx1DPtr idx, Idx1DPtr max_idx, NodeType nx) {
  return dense1DBlockMap(idx, max_idx, nx);
}

NodeType defaultDenseIndex2DMap(Idx2DPtr idx, Idx2DPtr max_idx, NodeType nx) {
  return dense2DBlockMap(idx, max_idx, nx);
}

NodeType defaultDenseIndex3DMap(Idx3DPtr idx, Idx3DPtr max_idx, NodeType nx) {
  return dense3DBlockMap(idx, max_idx, nx);
}

// Default round robin mappings
NodeType dense1DRoundRobinMap(Idx1DPtr idx, Idx1DPtr max_idx, NodeType nnodes) {
  return idx->x() % nnodes;
}

NodeType dense2DRoundRobinMap(Idx2DPtr idx, Idx2DPtr max_idx, NodeType nnodes) {
  using IndexElmType = typename Index2D::DenseIndexType;
  auto const& lin_idx = linearizeDenseIndexColMajor<IndexElmType, 2>(idx, max_idx);
  return lin_idx % nnodes;
}

NodeType dense3DRoundRobinMap(Idx3DPtr idx, Idx3DPtr max_idx, NodeType nnodes) {
  using IndexElmType = typename Index3D::DenseIndexType;
  auto const& lin_idx = linearizeDenseIndexColMajor<IndexElmType, 3>(idx, max_idx);
  return lin_idx % nnodes;
}

// Default block mappings
NodeType dense1DBlockMap(Idx1DPtr idx, Idx1DPtr max_idx, NodeType nnodes) {
  return denseBlockMap<Index1D, 1>(idx, max_idx, nnodes);
}

NodeType dense2DBlockMap(Idx2DPtr idx, Idx2DPtr max_idx, NodeType nnodes) {
  return denseBlockMap<Index2D, 2>(idx, max_idx, nnodes);
}

NodeType dense3DBlockMap(Idx3DPtr idx, Idx3DPtr max_idx, NodeType nnodes) {
  return denseBlockMap<Index3D, 3>(idx, max_idx, nnodes);
}

}}  // end namespace vt::mapping

