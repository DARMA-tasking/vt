/*
//@HEADER
// *****************************************************************************
//
//                              dense_array.impl.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_TOPOS_INDEX_DENSE_DENSE_ARRAY_IMPL_H
#define INCLUDED_TOPOS_INDEX_DENSE_DENSE_ARRAY_IMPL_H

#include "vt/config.h"
#include "vt/topos/index/dense/dense_array.h"
#include "vt/topos/index/base_index.h"
#include "vt/context/context.h"
#include "vt/utils/bits/bits_packer.h"

#include <functional>
#include <string>
#include <type_traits>
#include <sstream>
#include <array>

namespace vt { namespace index {

template <typename T, NumDimensionsType ndim>
DenseIndexArray<T, ndim>::DenseIndexArray(
  std::array<T, ndim> in_array
) : BaseIndex(), dims(in_array)
{ }

template <typename T, NumDimensionsType ndim>
template <typename... Idxs, typename>
DenseIndexArray<T, ndim>::DenseIndexArray(Idxs&&... init)
  : BaseIndex(), dims({{init...}})
{ }

template <typename T, NumDimensionsType ndim>
DenseIndexArray<T, ndim>::DenseIndexArray(
  DenseIndexArraySingleInitTag, T const& init_value
) : BaseIndex()
{
  for (int i = 0; i < ndim; i++) {
    dims[i] = init_value;
  }
}

template <typename T, NumDimensionsType ndim>
T& DenseIndexArray<T, ndim>::operator[](T const& index) {
  return dims[index];
}

template <typename T, NumDimensionsType ndim>
T const& DenseIndexArray<T, ndim>::operator[](
  T const& index
) const {
  return dims[index];
}

template <typename T, NumDimensionsType ndim>
T DenseIndexArray<T, ndim>::get(T const& index) const {
  return dims[index];
}

template <typename T, NumDimensionsType ndim>
typename DenseIndexArray<T, ndim>::IndexSizeType
DenseIndexArray<T, ndim>::packedSize() const {
  return ndim * sizeof(IndexType);
}

template <typename T, NumDimensionsType ndim>
bool DenseIndexArray<T, ndim>::indexIsByteCopyable() const {
  return true;
}

template <typename T, NumDimensionsType ndim>
void DenseIndexArray<T, ndim>::foreach(ApplyType fn) const {
  auto const idx = *this;
  return foreach(idx, fn);
}

template <typename T, NumDimensionsType ndim>
void DenseIndexArray<T, ndim>::foreach(IndexType in_max, ApplyType fn) const {
  IndexType max = in_max;
  auto size = max.getSize();
  std::array<T, ndim> vec = {{0}};
  for (decltype(size) sz = 0; sz < size; sz++) {
    fn(IndexType(vec));
    for (auto i = 0; i < ndim; i++) {
      if (vec[i] + 1 < max[i]) {
        vec[i]++;
        for (auto j = 0; j < i; j++) {
          vec[j] = 0;
        }
        break;
      }
    }
  }
}

template <typename T, NumDimensionsType ndim>
UniqueIndexBitType DenseIndexArray<T, ndim>::uniqueBits() const {
  UniqueIndexBitType bits{};
  auto const& nbits = (sizeof(UniqueIndexBitType) * 8) / ndim;
  for (auto i = 0; i < ndim; i++) {
    vt::utils::BitPacker::setFieldDynamic(i*nbits, nbits, bits, dims[i]);
  }
  return bits;
}

template <typename T, NumDimensionsType ndim>
/*static*/ typename DenseIndexArray<T, ndim>::IndexType
DenseIndexArray<T, ndim>::uniqueBitsToIndex(UniqueIndexBitType const& bits) {
  using BitType = UniqueIndexBitType;

  IndexType idx{};
  auto const& nbits = (sizeof(UniqueIndexBitType) * 8) / ndim;
  for (auto i = 0; i < ndim; i++) {
    auto const& val = vt::utils::BitPacker::getFieldDynamic<BitType>(i*nbits, nbits, bits);
    idx[i] = val;
  }
  return idx;
}

template <typename T, NumDimensionsType ndim>
typename DenseIndexArray<T, ndim>::DenseArraySizeType
DenseIndexArray<T, ndim>::getSize() const {
  DenseArraySizeType sz = 1;
  for (int i = 0; i < ndim; i++) {
    sz *= dims[i];
  }
  return sz;
}

template <typename T, NumDimensionsType ndim>
std::string DenseIndexArray<T, ndim>::toString() const {
  std::stringstream stream;
  stream << "[";
  for (int i = 0; i < ndim; i++) {
    stream << dims[i] << (i != ndim - 1 ? "," : "");
  }
  stream << "]";
  return stream.str();
}

template <typename T, NumDimensionsType ndim>
bool DenseIndexArray<T, ndim>::operator==(
  DenseIndexArrayType const& other
) const {
  for (int i = ndim - 1; i >= 0; i--) {
    if (dims[i] != other.dims[i]) {
      return false;
    }
  }
  return true;
}

template <typename T, NumDimensionsType ndim>
T const* DenseIndexArray<T, ndim>::raw() const {
  return &dims[0];
}

template <typename T, NumDimensionsType ndim>
bool DenseIndexArray<T, ndim>::operator<(
  DenseIndexArrayType const& other
) const {
  for (int i = ndim - 1; i >= 0; i--) {
    if (dims[i] < other.dims[i]) {
      return true;
    } else if (dims[i] > other.dims[i]) {
      return false;
    }
  }
  return false;
}

template <typename T, NumDimensionsType ndim>
typename DenseIndexArray<T, ndim>::DenseIndexArrayType
DenseIndexArray<T, ndim>::operator+(DenseIndexArrayType const& other) const {
  DenseIndexArrayType val;
  for (int i = 0; i < ndim; i++) {
    val.dims[i] = dims[i] + other.dims[i];
  }
  return val;
}

template <typename T, NumDimensionsType ndim>
typename DenseIndexArray<T, ndim>::DenseIndexArrayType
DenseIndexArray<T, ndim>::operator-(DenseIndexArrayType const& other) const {
  DenseIndexArrayType val;
  for (int i = 0; i < ndim; i++) {
    val.dims[i] = dims[i] - other.dims[i];
  }
  return val;
}

// special accessors (x,y,z) enabled depending on the number of dimensions
template <typename T, NumDimensionsType ndim>
template <typename U, typename>
T DenseIndexArray<T, ndim>::x() const {
  return dims[0];
}

template <typename T, NumDimensionsType ndim>
template <typename U, typename>
T DenseIndexArray<T, ndim>::y() const {
  return dims[1];
}

template <typename T, NumDimensionsType ndim>
template <typename U, typename>
T DenseIndexArray<T, ndim>::z() const {
  return dims[2];
}

template <typename IndexT, NumDimensionsType nd>
std::ostream& operator<<(
  std::ostream& os, ::vt::index::DenseIndexArray<IndexT,nd> const& idx
) {
  os << "idx(";
    for (int i = 0; i < nd; i++) {
      os << idx.dims[i];
      if (i != nd-1) {
        os << ",";
      }
    }
  os << ")";
  return os;
}

}}  // end namespace vt::index

namespace std {
  template <typename T, ::vt::index::NumDimensionsType ndim>
  struct hash<::vt::index::DenseIndexArray<T, ndim>> {
    size_t operator()(
      ::vt::index::DenseIndexArray<T, ndim> const& in
    ) const {
      auto val = std::hash<T>()(in[0]);
      for (auto i = 1; i < ndim; i++) {
        val += std::hash<T>()(in[i]);
      }
      return val;
    }
  };
}

#endif /*INCLUDED_TOPOS_INDEX_DENSE_DENSE_ARRAY_IMPL_H*/
