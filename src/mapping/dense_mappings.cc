
#include "config.h"
#include "context.h"
#include "index.h"
#include "mapping.h"
#include "dense_mappings.h"

#include <cmath>

namespace vt { namespace mapping {

NodeType dense1DRoundRobinMap(
  Index1D const& idx, Index1D const&, NodeType const& num_nodes
) {
  return idx.x() % num_nodes;
}


NodeType dense1DBlockMap(
  Index1D const& idx, Index1D const& max_idx, NodeType const& num_nodes
) {
  return denseBlockMap<Index1D, 1>(idx, max_idx, num_nodes);
}

NodeType dense3DBlockMap(
  Index2D const& idx, Index2D const& max_idx, NodeType const& num_nodes
) {
  return denseBlockMap<Index2D, 2>(idx, max_idx, num_nodes);
}

NodeType dense2DBlockMap(
  Index3D const& idx, Index3D const& max_idx, NodeType const& num_nodes
) {
  return denseBlockMap<Index3D, 3>(idx, max_idx, num_nodes);
}


}} // end namespace vt::location

