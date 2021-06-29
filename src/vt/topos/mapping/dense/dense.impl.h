/*
//@HEADER
// *****************************************************************************
//
//                                 dense.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_TOPOS_MAPPING_DENSE_DENSE_IMPL_H
#define INCLUDED_VT_TOPOS_MAPPING_DENSE_DENSE_IMPL_H

#include <cmath>

#include "vt/config.h"
#include "vt/topos/mapping/mapping.h"
#include "vt/topos/mapping/dense/dense.h"
#include "vt/topos/index/index.h"

namespace vt { namespace mapping {

template <typename T>
NodeType defaultDenseIndex1DMap(Idx1DPtr<T> idx, Idx1DPtr<T> max, NodeType nx) {
  return dense1DBlockMap<T>(idx, max, nx);
}

template <typename T>
NodeType defaultDenseIndex2DMap(Idx2DPtr<T> idx, Idx2DPtr<T> max, NodeType nx) {
  return dense2DBlockMap<T>(idx, max, nx);
}

template <typename T>
NodeType defaultDenseIndex3DMap(Idx3DPtr<T> idx, Idx3DPtr<T> max, NodeType nx) {
  return dense3DBlockMap<T>(idx, max, nx);
}

// Default round robin mappings
template <typename T>
NodeType dense1DRoundRobinMap(Idx1DPtr<T> idx, Idx1DPtr<T> max, NodeType nx) {
  return idx->x() % nx;
}

template <typename T>
NodeType dense2DRoundRobinMap(Idx2DPtr<T> idx, Idx2DPtr<T> max, NodeType nx) {
  using IndexElmType = typename IdxType2D<T>::DenseIndexType;
  auto const& lin_idx = linearizeDenseIndexColMajor<IndexElmType, 2>(idx, max);
  return lin_idx % nx;
}

template <typename T>
NodeType dense3DRoundRobinMap(Idx3DPtr<T> idx, Idx3DPtr<T> max, NodeType nx) {
  using IndexElmType = typename IdxType3D<T>::DenseIndexType;
  auto const& lin_idx = linearizeDenseIndexColMajor<IndexElmType, 3>(idx, max);
  return lin_idx % nx;
}

// Default block mappings
template <typename T>
NodeType dense1DBlockMap(Idx1DPtr<T> idx, Idx1DPtr<T> max, NodeType nx) {
  return denseBlockMap<IdxType1D<T>, 1>(idx, max, nx);
}

template <typename T>
NodeType dense2DBlockMap(Idx2DPtr<T> idx, Idx2DPtr<T> max, NodeType nx) {
  return denseBlockMap<IdxType2D<T>, 2>(idx, max, nx);
}

template <typename T>
NodeType dense3DBlockMap(Idx3DPtr<T> idx, Idx3DPtr<T> max, NodeType nx) {
  return denseBlockMap<IdxType3D<T>, 3>(idx, max, nx);
}

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
  IndexElmType const& bin_size_floor = static_cast<IndexElmType>(bin_floor_dbl);
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
Idx linearizeDenseIndexColMajor(
  DenseIndex <Idx, ndim> *idx, DenseIndex <Idx, ndim> *max_idx
) {
  // @todo: Do we have a defined behaviour when index is out of max_idx?
  // @todo: This might be useful mechanism to express a boundary condition

  auto const& idx_ = *idx;
  auto const& max_idx_ = *max_idx;

  for (size_t dim = 0; dim < ndim; dim++) {
    vtAssert(idx_[dim] <  max_idx_[dim], "Out of range index!");
  }

  Idx val = 0;
  Idx dim_size = 1;
  for (auto i = ndim - 1; i >= 0; i--) {
    val += dim_size * idx_[i];
    dim_size *= max_idx_[i];
  }
  return val;
}

template <typename Idx, index::NumDimensionsType ndim>
Idx linearizeDenseIndexRowMajor(
  DenseIndex <Idx, ndim> *idx, DenseIndex <Idx, ndim> *max_idx
) {
  // @todo: Do we have a defined behaviour when index is out of max_idx?
  // @todo: This might be useful mechanism to express a boundary condition

  auto const& idx_ = *idx;
  auto const& max_idx_ = *max_idx;

  for (size_t dim = 0; dim < ndim; dim++) {
    vtAssert(idx_[dim] <  max_idx_[dim], "Out of range index!");
  }

  Idx val = 0;
  Idx dim_size = 1;
  for (auto i = 0; i < ndim; i++) {
    val += dim_size * idx_[i];
    dim_size *= max_idx_[i];
  }
  return val;
}

template <typename Idx, index::NumDimensionsType ndim>
NodeType denseBlockMap(IdxPtr<Idx> idx, IdxPtr<Idx> max_idx, NodeType nnodes) {
  using IndexElmType = typename Idx::DenseIndexType;

  IndexElmType total_elems = max_idx->getSize();
  IndexElmType flat_idx = linearizeDenseIndexColMajor<IndexElmType, ndim>(
    idx, max_idx
  );

  return blockMapDenseFlatIndex<IndexElmType, NodeType>(
    &flat_idx, &total_elems, nnodes
  );
}

}}  // end namespace vt::mapping

#endif /*INCLUDED_VT_TOPOS_MAPPING_DENSE_DENSE_IMPL_H*/
