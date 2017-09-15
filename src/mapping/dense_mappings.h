
#if ! defined __RUNTIME_TRANSPORT_DENSE_MAPPING__
#define __RUNTIME_TRANSPORT_DENSE_MAPPING__

#include "config.h"
#include "index.h"
#include "mapping.h"

#include <functional>

namespace vt { namespace mapping {

template <typename IndexElmType, typename PhysicalType>
NodeType blockMapDenseFlatIndex(
  IndexElmType const& flat_idx, IndexElmType const& num_elems,
  PhysicalType const& num_resources
);

template <typename IndexType, typename IndexElmType>
IndexElmType flattenIndexND(IndexType const& idx, IndexType const& max);

NodeType dense1DRoundRobinMap(Index1D const& idx, NodeType const& num_nodes);
NodeType defaultDenseIndex1DMap(Index1D const& idx, NodeType const& num_nodes);

NodeType dense1DRoundRobinMap(
  Index1D const& idx, Index1D const&, NodeType const& num_nodes
);

template <typename Index, index::NumDimensionsType ndim>
NodeType denseBlockMap(
  Index const& idx, Index const& max_idx, NodeType const& num_nodes
);

NodeType dense1DBlockMap(
  Index1D const& idx, Index1D const& max_idx, NodeType const& num_nodes
);
NodeType dense3DBlockMap(
  Index2D const& idx, Index2D const& max_idx, NodeType const& num_nodes
);
NodeType dense2DBlockMap(
  Index3D const& idx, Index3D const& max_idx, NodeType const& num_nodes
);


}} // end namespace vt::location

#include "dense_mappings.impl.h"

#endif /*__RUNTIME_TRANSPORT_DENSE_MAPPING__*/
