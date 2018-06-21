
#if !defined INCLUDED_TOPOS_INDEX_DENSE_DENSE_ARRAY_IMPL_H
#define INCLUDED_TOPOS_INDEX_DENSE_DENSE_ARRAY_IMPL_H

#include "config.h"
#include "topos/index/dense/dense_array.h"
#include "topos/index/base_index.h"
#include "context/context.h"
#include "utils/bits/bits_packer.h"

#include <functional>
#include <string>
#include <type_traits>
#include <sstream>
#include <array>

namespace vt { namespace index {

template <typename IndexType, NumDimensionsType ndim>
DenseIndexArray<IndexType, ndim>::DenseIndexArray(
  std::array<IndexType, ndim> in_array
) : dims(in_array), BaseIndex()
{ }

template <typename IndexType, NumDimensionsType ndim>
DenseIndexArray<IndexType, ndim>::DenseIndexArray(
  std::initializer_list<IndexType> vals
) : dims{vals}, BaseIndex()
{ }

template <typename IndexType, NumDimensionsType ndim>
template <typename... Idxs, typename>
DenseIndexArray<IndexType, ndim>::DenseIndexArray(Idxs&&... init)
  : dims({init...}), BaseIndex()
{ }

template <typename IndexType, NumDimensionsType ndim>
DenseIndexArray<IndexType, ndim>::DenseIndexArray(
  DenseIndexArraySingleInitTag, IndexType const& init_value
) : BaseIndex()
{
  for (int i = 0; i < ndim; i++) {
    dims[i] = init_value;
  }
}

template <typename IndexType, NumDimensionsType ndim>
IndexType& DenseIndexArray<IndexType, ndim>::operator[](IndexType const& index) {
  return dims[index];
}

template <typename IndexType, NumDimensionsType ndim>
IndexType const& DenseIndexArray<IndexType, ndim>::operator[](
  IndexType const& index
) const {
  return dims[index];
}

template <typename IndexType, NumDimensionsType ndim>
IndexType DenseIndexArray<IndexType, ndim>::get(IndexType const& index) const {
  return dims[index];
}

template <typename IndexType, NumDimensionsType ndim>
typename DenseIndexArray<IndexType, ndim>::IndexSizeType
DenseIndexArray<IndexType, ndim>::packedSize() const {
  return ndim * sizeof(IndexType);
}

template <typename IndexType, NumDimensionsType ndim>
bool DenseIndexArray<IndexType, ndim>::indexIsByteCopyable() const {
  return true;
}

template <typename IndexType, NumDimensionsType ndim>
void DenseIndexArray<IndexType, ndim>::foreach(
  ThisIndexType in_max, ApplyType fn
) const {
  ThisIndexType idx;
  ThisIndexType max = in_max;
  std::array<IndexType, ndim> vec = {0};
  for (auto sz = 0; sz < max.getSize(); sz++) {
    fn(ThisIndexType(vec));
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

template <typename IndexType, NumDimensionsType ndim>
UniqueIndexBitType DenseIndexArray<IndexType, ndim>::uniqueBits() const {
  UniqueIndexBitType bits{};
  auto const& nbits = (sizeof(UniqueIndexBitType) * 8) / ndim;
  for (auto i = 0; i < ndim; i++) {
    vt::utils::BitPacker::setFieldDynamic(i*nbits, nbits, bits, dims[i]);
  }
  return bits;
}

template <typename IndexType, NumDimensionsType ndim>
/*static*/ typename DenseIndexArray<IndexType, ndim>::ThisIndexType
DenseIndexArray<IndexType, ndim>::uniqueBitsToIndex(UniqueIndexBitType const& bits) {
  using BitType = UniqueIndexBitType;

  ThisIndexType idx{};
  auto const& nbits = (sizeof(UniqueIndexBitType) * 8) / ndim;
  for (auto i = 0; i < ndim; i++) {
    auto const& val = vt::utils::BitPacker::getFieldDynamic<BitType>(i*nbits, nbits, bits);
    idx[i] = val;
  }
  return idx;
}

template <typename IndexType, NumDimensionsType ndim>
typename DenseIndexArray<IndexType, ndim>::DenseArraySizeType
DenseIndexArray<IndexType, ndim>::getSize() const {
  DenseArraySizeType sz = 1;
  for (int i = 0; i < ndim; i++) {
    sz *= dims[i];
  }
  return sz;
}

template <typename IndexType, NumDimensionsType ndim>
std::string DenseIndexArray<IndexType, ndim>::toString() const {
  std::stringstream stream;
  stream << "[";
  for (int i = 0; i < ndim; i++) {
    stream << dims[i] << (i != ndim - 1 ? "," : "");
  }
  stream << "]";
  return stream.str();
}

template <typename IndexType, NumDimensionsType ndim>
bool DenseIndexArray<IndexType, ndim>::operator==(
  DenseIndexArrayType const& other
) const {
  for (int i = ndim - 1; i >= 0; i--) {
    if (dims[i] != other.dims[i]) {
      return false;
    }
  }
  return true;
}

template <typename IndexType, NumDimensionsType ndim>
IndexType const* DenseIndexArray<IndexType, ndim>::raw() const {
  return &dims[0];
}

template <typename IndexType, NumDimensionsType ndim>
bool DenseIndexArray<IndexType, ndim>::operator<(
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

template <typename IndexType, NumDimensionsType ndim>
typename DenseIndexArray<IndexType, ndim>::DenseIndexArrayType
DenseIndexArray<IndexType, ndim>::operator+(DenseIndexArrayType const& other) const {
  DenseIndexArrayType val;
  for (int i = 0; i < ndim; i++) {
    val.dims[i] = dims[i] + other.dims[i];
  }
  return val;
}

template <typename IndexType, NumDimensionsType ndim>
typename DenseIndexArray<IndexType, ndim>::DenseIndexArrayType
DenseIndexArray<IndexType, ndim>::operator-(DenseIndexArrayType const& other) const {
  DenseIndexArrayType val;
  for (int i = 0; i < ndim; i++) {
    val.dims[i] = dims[i] - other.dims[i];
  }
  return val;
}

// special accessors (x,y,z) enabled depending on the number of dimensions
template <typename IndexType, NumDimensionsType ndim>
template <typename T, typename>
IndexType DenseIndexArray<IndexType, ndim>::x() const {
  return dims[0];
}

template <typename IndexType, NumDimensionsType ndim>
template <typename T, typename>
IndexType DenseIndexArray<IndexType, ndim>::y() const {
  return dims[1];
}

template <typename IndexType, NumDimensionsType ndim>
template <typename T, typename>
IndexType DenseIndexArray<IndexType, ndim>::z() const {
  return dims[2];
}

}}  // end namespace vt::index

namespace std {
  template <typename IndexType, ::vt::index::NumDimensionsType ndim>
  struct hash<::vt::index::DenseIndexArray<IndexType, ndim>> {
    size_t operator()(
      ::vt::index::DenseIndexArray<IndexType, ndim> const& in
    ) const {
      auto val = std::hash<IndexType>()(in[0]);
      for (auto i = 1; i < ndim; i++) {
        val += std::hash<IndexType>()(in[i]);
      }
      return val;
    }
  };
}

#endif /*INCLUDED_TOPOS_INDEX_DENSE_DENSE_ARRAY_IMPL_H*/
