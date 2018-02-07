
#if !defined INCLUDED_TOPOS_INDEX_DENSE_ARRAY
#define INCLUDED_TOPOS_INDEX_DENSE_ARRAY

#include <array>
#include <type_traits>
#include <string>
#include <sstream>
#include <cstdint>

#include "config.h"
#include "utils/bits/bits_packer.h"

#if backend_check_enabled(detector)
#include "topos/index/index_traits.h"
#endif

namespace vt { namespace index {

using NumDimensionsType = int8_t;

template <typename IndexType, NumDimensionsType ndim = 1>
struct DenseIndexArray;

template <typename IndexType, NumDimensionsType ndim>
struct DenseIndexArray {
  using ThisIndexType = DenseIndexArray<IndexType, ndim>;
  using IndexSizeType = size_t;

  struct dense_single_value_tag {};

  using DenseIndexArrayType = DenseIndexArray<IndexType, ndim>;
  using DenseArraySizeType = uint64_t;
  using DenseIndexType = IndexType;
  using isByteCopyable = std::true_type;

  std::array<IndexType, ndim> dims = {};

  DenseIndexArray() = default;
  DenseIndexArray(DenseIndexArray const&) = default;
  DenseIndexArray& operator=(DenseIndexArray const&) = default;
  DenseIndexArray(DenseIndexArray&&) = default;

  template <typename... Idxs>
  explicit DenseIndexArray(Idxs&&... init) : dims({init...}) {}

  DenseIndexArray(dense_single_value_tag, IndexType const& init_value) {
    for (int i = 0; i < ndim; i++) {
      dims[i] = init_value;
    }
  }

  IndexType& operator[](IndexType const& index) {
    return dims[index];
  }

  IndexType get(IndexType const& index) const {
    return dims[index];
  }

  IndexSizeType packedSize() const {
    return ndim * sizeof(IndexType);
  }

  bool indexIsByteCopyable() const {
    return true;
  }

  void foreach(ThisIndexType max, std::function<void(ThisIndexType)> fn) {
    ThisIndexType idx;
    std::array<IndexType, ndim> vec;
    printf("size=%llu\n",max.getSize());
    for (auto sz = 0; sz < max.getSize(); sz++) {
      for (auto i = 0; i < ndim; i++) {
        if (idx[i] + 1 <= max[idx[i]]) {
          vec[i]++;
          break;
        }
      }
      fn(ThisIndexType(vec));
    }
  }

  UniqueIndexBitType uniqueBits() const {
    UniqueIndexBitType bits{};
    auto const& nbits = (sizeof(UniqueIndexBitType) * 8) / ndim;
    for (auto i = 0; i < ndim; i++) {
      vt::utils::BitPacker::setFieldDynamic(i*nbits, nbits, bits, dims[i]);
    }
    return bits;
  }

  static ThisIndexType uniqueBitsToIndex(UniqueIndexBitType const& bits) {
    using BitType = UniqueIndexBitType;

    ThisIndexType idx{};
    auto const& nbits = (sizeof(UniqueIndexBitType) * 8) / ndim;
    for (auto i = 0; i < ndim; i++) {
      auto const& val = vt::utils::BitPacker::getFieldDynamic<BitType>(i*nbits, nbits, bits);
      idx[i] = val;
    }
    return idx;
  }

  DenseArraySizeType getSize() const {
    DenseArraySizeType sz = 1;
    for (int i = 0; i < ndim; i++) {
      sz *= dims[i];
    }
    return sz;
  }

  std::string toString() const {
    std::stringstream stream;
    stream << "[";
    for (int i = 0; i < ndim; i++) {
      stream << dims[i] << (i != ndim - 1 ? "," : "");
    }
    stream << "]";
    return stream.str();
  }

  bool operator==(DenseIndexArrayType const& other) const {
    for (int i = ndim - 1; i >= 0; i--) {
      if (dims[i] != other.dims[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator<(DenseIndexArrayType const& other) const {
    for (int i = ndim - 1; i >= 0; i--) {
      if (dims[i] < other.dims[i]) {
        return true;
      } else if (dims[i] > other.dims[i]) {
        return false;
      }
    }
    return false;
  }

  // operator + and - for accessing neighboring indices
  DenseIndexArrayType operator+(DenseIndexArrayType const& other) const {
    DenseIndexArrayType val;
    for (int i = 0; i < ndim; i++) {
      val.dims[i] = dims[i] + other.dims[i];
    }
    return val;
  }

  DenseIndexArrayType operator-(DenseIndexArrayType const& other) const {
    DenseIndexArrayType val;
    for (int i = 0; i < ndim; i++) {
      val.dims[i] = dims[i] - other.dims[i];
    }
    return val;
  }

  // special accessors (x,y,z) enabled depending on the number of dimensions
  template <
      typename T = void,
      typename = typename std::enable_if<ndim >= 1, T>::type
  >
  IndexType x() const { return dims[0]; }

  template <
      typename T = void,
      typename = typename std::enable_if<ndim >= 2, T>::type
  >
  IndexType y() const { return dims[1]; }

  template <
      typename T = void,
      typename = typename std::enable_if<ndim >= 3, T>::type
  >
  IndexType z() const { return dims[2]; }
};

#if backend_check_enabled(detector)
static_assert(
  vt::index::IndexTraits<DenseIndexArray<int, 10>>::is_index,
  "DenseIndexArray must follow the index concept"
);
#endif

}}  // end namespace vt::index

#endif  /*INCLUDED_TOPOS_INDEX_DENSE_ARRAY*/
