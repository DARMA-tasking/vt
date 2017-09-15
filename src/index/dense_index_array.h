
#if ! defined __RUNTIME_TRANSPORT_DENSE_INDEX_ARRAY__
#define __RUNTIME_TRANSPORT_DENSE_INDEX_ARRAY__

#include "config.h"
#include "base_index.h"

#include <array>
#include <string>
#include <sstream>
#include <cstdint>

namespace vt { namespace index {

using NumDimensionsType = int8_t;

template <typename IndexType, NumDimensionsType ndim = 1>
struct DenseIndexArray : BaseIndex {
  struct dense_single_value_tag { };

  using DenseIndexArrayType = DenseIndexArray<IndexType, ndim>;
  using DenseArraySizeType = uint64_t;
  using DenseIndexType = IndexType;

  std::array<IndexType, ndim> dims = {};

  DenseIndexArray() = default;
  DenseIndexArray(DenseIndexArray const&) = default;
  DenseIndexArray(DenseIndexArray&&) = default;

  template <typename... Idxs>
  explicit DenseIndexArray(Idxs&&... init) : BaseIndex(), dims({init...}) { }

  DenseIndexArray(dense_single_value_tag, IndexType const& init_value) : BaseIndex() {
    for (int i = 0; i < ndim; i++) {
      dims[i] = init_value;
    }
  }

  IndexType operator[](IndexType const& index) const {
    return dims[index];
  }

  IndexSizeType packedSize() const {
    return ndim * sizeof(IndexType);
  }

  bool isByteCopyable() const {
    return true;
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
      stream << dims[i] << (i != ndim-1 ? "," : "");
    }
    stream << "]";
    return stream.str();
  }

  bool operator==(DenseIndexArrayType const& other) const {
    for (int i = ndim-1; i >= 0; i--) {
      if (dims[i] != other.dims[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator<(DenseIndexArrayType const& other) const {
    for (int i = ndim-1; i >= 0; i--) {
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

}} // end namespace vt::index

#endif /*__RUNTIME_TRANSPORT_DENSE_INDEX_ARRAY__*/
