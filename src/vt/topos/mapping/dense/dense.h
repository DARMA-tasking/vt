
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

template <typename T = IdxBase> using Idx1DPtr = IdxType1D<T>*;
template <typename T = IdxBase> using Idx2DPtr = IdxType2D<T>*;
template <typename T = IdxBase> using Idx3DPtr = IdxType3D<T>*;

template <typename Idx, index::NumDimensionsType ndim>
NodeType denseBlockMap(IdxPtr<Idx> idx, IdxPtr<Idx> max_idx, NodeType nnodes);

template <typename T = IdxBase>
NodeType defaultDenseIndex1DMap(Idx1DPtr<T> idx, Idx1DPtr<T> max, NodeType n);
template <typename T = IdxBase>
NodeType defaultDenseIndex2DMap(Idx2DPtr<T> idx, Idx2DPtr<T> max, NodeType n);
template <typename T = IdxBase>
NodeType defaultDenseIndex3DMap(Idx3DPtr<T> idx, Idx3DPtr<T> max, NodeType n);

template <typename T = IdxBase>
NodeType dense1DRoundRobinMap(  Idx1DPtr<T> idx, Idx1DPtr<T> max, NodeType n);
template <typename T = IdxBase>
NodeType dense2DRoundRobinMap(  Idx2DPtr<T> idx, Idx2DPtr<T> max, NodeType n);
template <typename T = IdxBase>
NodeType dense3DRoundRobinMap(  Idx3DPtr<T> idx, Idx3DPtr<T> max, NodeType n);

template <typename T = IdxBase>
NodeType dense1DBlockMap(       Idx1DPtr<T> idx, Idx1DPtr<T> max, NodeType n);
template <typename T = IdxBase>
NodeType dense2DBlockMap(       Idx2DPtr<T> idx, Idx2DPtr<T> max, NodeType n);
template <typename T = IdxBase>
NodeType dense3DBlockMap(       Idx3DPtr<T> idx, Idx3DPtr<T> max, NodeType n);

template <typename T = IdxBase>   using i1D   = IdxType1D<T>;
template <typename T = IdxBase>   using i2D   = IdxType2D<T>;
template <typename T = IdxBase>   using i3D   = IdxType3D<T>;
template <typename F, F* f>       using Adapt = FunctorAdapt<F,f>;

template <typename T = IdxBase>
using dense1DMapFn    = Adapt<MapAdapter<i1D<T>>, defaultDenseIndex1DMap<T>>;
template <typename T = IdxBase>
using dense2DMapFn    = Adapt<MapAdapter<i2D<T>>, defaultDenseIndex2DMap<T>>;
template <typename T = IdxBase>
using dense3DMapFn    = Adapt<MapAdapter<i3D<T>>, defaultDenseIndex3DMap<T>>;
template <typename T = IdxBase>
using dense1DRRMapFn  = Adapt<MapAdapter<i1D<T>>, dense1DRoundRobinMap<T>>;
template <typename T = IdxBase>
using dense2DRRMapFn  = Adapt<MapAdapter<i2D<T>>, dense2DRoundRobinMap<T>>;
template <typename T = IdxBase>
using dense3DRRMapFn  = Adapt<MapAdapter<i3D<T>>, dense3DRoundRobinMap<T>>;
template <typename T = IdxBase>
using dense1DBlkMapFn = Adapt<MapAdapter<i1D<T>>, dense1DBlockMap<T>>;
template <typename T = IdxBase>
using dense2DBlkMapFn = Adapt<MapAdapter<i2D<T>>, dense2DBlockMap<T>>;
template <typename T = IdxBase>
using dense3DBlkMapFn = Adapt<MapAdapter<i3D<T>>, dense3DBlockMap<T>>;

}}  // end namespace vt::mapping

#include "vt/topos/mapping/dense/dense.impl.h"

#endif /*INCLUDED_TOPOS_MAPPING_DENSE*/
