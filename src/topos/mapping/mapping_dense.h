
#if !defined INCLUDED_TOPOS_MAPPING_DENSE
#define INCLUDED_TOPOS_MAPPING_DENSE

#include <functional>

#include "config.h"
#include "topos/index/index.h"
#include "mapping.h"

namespace vt { namespace mapping {

template <typename IndexElmType, typename PhysicalType>
NodeType blockMapDenseFlatIndex(
    IndexElmType const& flat_idx, IndexElmType const& num_elems,
    PhysicalType const& num_resources
);

template <typename Idx, index::NumDimensionsType ndim>
Idx linearizeDenseIndex(
    DenseIndex<Idx, ndim> const& idx, DenseIndex<Idx, ndim> const& max_idx
);

template <typename Index>
using IdxRef = Index const&;
using Idx1DRef = Index1D const&;
using Idx2DRef = Index2D const&;
using Idx3DRef = Index3D const&;
using NodeRef = NodeType const&;

template <typename Idx, index::NumDimensionsType ndim>
NodeType denseBlockMap(IdxRef<Idx> idx, IdxRef<Idx> max_idx, NodeRef nnodes);

NodeType defaultDenseIndex1DMap(Idx1DRef idx, Idx1DRef max_idx, NodeRef nnodes);
NodeType defaultDenseIndex2DMap(Idx2DRef idx, Idx2DRef max_idx, NodeRef nnodes);
NodeType defaultDenseIndex3DMap(Idx3DRef idx, Idx3DRef max_idx, NodeRef nnodes);

NodeType dense1DRoundRobinMap(Idx1DRef idx, Idx1DRef max_idx, NodeRef nnodes);
NodeType dense2DRoundRobinMap(Idx2DRef idx, Idx2DRef max_idx, NodeRef nnodes);
NodeType dense3DRoundRobinMap(Idx3DRef idx, Idx3DRef max_idx, NodeRef nnodes);

NodeType dense1DBlockMap(Idx1DRef idx, Idx1DRef max_idx, NodeRef nnodes);
NodeType dense2DBlockMap(Idx2DRef idx, Idx2DRef max_idx, NodeRef nnodes);
NodeType dense3DBlockMap(Idx3DRef idx, Idx3DRef max_idx, NodeRef nnodes);

}}  // end namespace vt::location

#include "mapping_dense.impl.h"

#endif /*INCLUDED_TOPOS_MAPPING_DENSE*/
