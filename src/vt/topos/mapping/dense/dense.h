
#if !defined INCLUDED_TOPOS_MAPPING_DENSE
#define INCLUDED_TOPOS_MAPPING_DENSE

#include "vt/config.h"
#include "vt/topos/mapping/mapping.h"
#include "vt/topos/mapping/adapt_mappers.h"
#include "vt/topos/index/index.h"

#include <functional>

namespace vt { namespace mapping {

template <typename IndexElmType, typename PhysicalType>
NodeType blockMapDenseFlatIndex(
  IndexElmType* flat_idx, IndexElmType* num_elems, PhysicalType num_resources
);

template <typename Idx, index::NumDimensionsType ndim>
Idx linearizeDenseIndexColMajor(
  DenseIndex<Idx, ndim> *idx, DenseIndex<Idx, ndim> *max_idx
);

template <typename Idx, index::NumDimensionsType ndim>
Idx linearizeDenseIndexRowMajor(
  DenseIndex<Idx, ndim> *idx, DenseIndex<Idx, ndim> *max_idx
);

template <typename Index>
using IdxPtr = Index*;

using Idx1DPtr = Index1D*;
using Idx2DPtr = Index2D*;
using Idx3DPtr = Index3D*;

template <typename Idx, index::NumDimensionsType ndim>
NodeType denseBlockMap(IdxPtr<Idx> idx, IdxPtr<Idx> max_idx, NodeType nnodes);

NodeType defaultDenseIndex1DMap(Idx1DPtr idx, Idx1DPtr max_idx, NodeType nnodes);
NodeType defaultDenseIndex2DMap(Idx2DPtr idx, Idx2DPtr max_idx, NodeType nnodes);
NodeType defaultDenseIndex3DMap(Idx3DPtr idx, Idx3DPtr max_idx, NodeType nnodes);

NodeType dense1DRoundRobinMap(Idx1DPtr idx, Idx1DPtr max_idx, NodeType nnodes);
NodeType dense2DRoundRobinMap(Idx2DPtr idx, Idx2DPtr max_idx, NodeType nnodes);
NodeType dense3DRoundRobinMap(Idx3DPtr idx, Idx3DPtr max_idx, NodeType nnodes);

NodeType dense1DBlockMap(Idx1DPtr idx, Idx1DPtr max_idx, NodeType nnodes);
NodeType dense2DBlockMap(Idx2DPtr idx, Idx2DPtr max_idx, NodeType nnodes);
NodeType dense3DBlockMap(Idx3DPtr idx, Idx3DPtr max_idx, NodeType nnodes);

using dense1DMapFn    = FunctorAdapt<MapAdapter<Index1D>, defaultDenseIndex1DMap>;
using dense2DMapFn    = FunctorAdapt<MapAdapter<Index2D>, defaultDenseIndex2DMap>;
using dense3DMapFn    = FunctorAdapt<MapAdapter<Index3D>, defaultDenseIndex3DMap>;
using dense1DRRMapFn  = FunctorAdapt<MapAdapter<Index1D>, dense1DRoundRobinMap>;
using dense2DRRMapFn  = FunctorAdapt<MapAdapter<Index2D>, dense2DRoundRobinMap>;
using dense3DRRMapFn  = FunctorAdapt<MapAdapter<Index3D>, dense3DRoundRobinMap>;
using dense1DBlkMapFn = FunctorAdapt<MapAdapter<Index1D>, dense1DBlockMap>;
using dense2DBlkMapFn = FunctorAdapt<MapAdapter<Index2D>, dense2DBlockMap>;
using dense3DBlkMapFn = FunctorAdapt<MapAdapter<Index3D>, dense3DBlockMap>;

}}  // end namespace vt::mapping

#include "vt/topos/mapping/dense/dense.impl.h"

#endif /*INCLUDED_TOPOS_MAPPING_DENSE*/
