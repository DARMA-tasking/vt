
#if !defined INCLUDED_TOPOS_MAPPING_DENSE_IMPL
#define INCLUDED_TOPOS_MAPPING_DENSE_IMPL

#include <cmath>

#include "config.h"
#include "topos/index/index.h"
#include "mapping.h"

namespace vt { namespace mapping {

template <typename IndexElmType, typename PhysicalType>
inline NodeType blockMapDenseFlatIndex(
  IndexElmType* flat_idx_ptr, IndexElmType* num_elems_ptr,
  PhysicalType num_resources
) {
  IndexElmType flat_idx = *flat_idx_ptr;
  IndexElmType num_elems = *num_elems_ptr;

  double const& elms_as_dbl = static_cast<double>(num_elems);
  double const& res_as_dbl = static_cast<double>(num_resources);
  double const& bin_floor_dbl = std::floor(elms_as_dbl / res_as_dbl);
  double const& bin_ceil_dbl = std::floor(elms_as_dbl / res_as_dbl);
  IndexElmType const& bin_size_floor = static_cast<IndexElmType>(bin_floor_dbl);
  IndexElmType const& bin_size_ceil = static_cast<IndexElmType>(bin_ceil_dbl);
  IndexElmType const& rem_elms = num_elems % num_resources;
  IndexElmType const& num_first_set = rem_elms * (bin_size_floor + 1);

  if (flat_idx < num_first_set) {
    return (flat_idx / (bin_size_floor + 1));
  } else if (flat_idx < num_elems) {
    return (rem_elms + (flat_idx - num_first_set) / bin_size_floor);
  } else {
    return flat_idx % num_resources;
  }
}

template <typename Idx, index::NumDimensionsType ndim>
Idx linearizeDenseIndex(
  DenseIndex<Idx, ndim>* idx, DenseIndex<Idx, ndim>* max_idx
) {
  Idx val = 0;
  Idx dim_size = 1;
  for (auto i = ndim - 1; i >= 0; i--) {
    val += dim_size * idx->dims[i];
    dim_size *= max_idx->dims[i];
  }
  return val;
}

template <typename Idx, index::NumDimensionsType ndim>
NodeType denseBlockMap(IdxPtr<Idx> idx, IdxPtr<Idx> max_idx, NodeType nnodes) {
  using IndexElmType = typename Idx::DenseIndexType;

  IndexElmType total_elems = max_idx->getSize();
  IndexElmType flat_idx = linearizeDenseIndex<IndexElmType, ndim>(idx, max_idx);

  return blockMapDenseFlatIndex<IndexElmType, NodeType>(
    &flat_idx, &total_elems, nnodes
  );
}

}}  // end namespace vt::location

#endif /*INCLUDED_TOPOS_MAPPING_DENSE_IMPL*/
