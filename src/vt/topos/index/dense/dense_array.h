
#if !defined INCLUDED_TOPOS_INDEX_DENSE_DENSE_ARRAY_H
#define INCLUDED_TOPOS_INDEX_DENSE_DENSE_ARRAY_H

#include "vt/config.h"
#include "vt/topos/index/base_index.h"
#include "vt/utils/bits/bits_packer.h"
#include "vt/utils/static_checks/meta_type_eq_.h"
#include "vt/serialization/traits/byte_copy_trait.h"

#include <array>
#include <type_traits>
#include <string>
#include <sstream>
#include <cstdint>
#include <functional>
#include <utility>
#include <initializer_list>
#include <ostream>

#if backend_check_enabled(detector)
  #include "vt/topos/index/traits/traits.h"
#endif

namespace vt { namespace index {

using NumDimensionsType = int8_t;

template <typename IndexType, NumDimensionsType ndim = 1>
struct DenseIndexArray;

static struct DenseIndexArraySingleInitTag { } dense_single_value_tag { };

template <typename IndexType, NumDimensionsType ndim>
struct DenseIndexArray : BaseIndex, serialization::ByteCopyTrait {
  using ThisIndexType = DenseIndexArray<IndexType, ndim>;
  using IndexSizeType = size_t;
  using ApplyType = std::function<void(ThisIndexType)>;
  using IsByteCopyable = serialization::ByteCopyTrait;
  using DenseIndexArrayType = DenseIndexArray<IndexType, ndim>;
  using DenseArraySizeType = uint64_t;
  using DenseIndexType = IndexType;

  DenseIndexArray() = default;
  DenseIndexArray(DenseIndexArray const&) = default;
  DenseIndexArray& operator=(DenseIndexArray const&) = default;
  DenseIndexArray(DenseIndexArray&&) = default;

  template <
    typename... Idxs,
    typename = typename std::enable_if<
      util::meta_type_eq<IndexType, typename std::decay<Idxs>::type...>::value
    >::type
  >
  explicit DenseIndexArray(Idxs&&... init);

  DenseIndexArray(std::initializer_list<IndexType> vals);
  DenseIndexArray(std::array<IndexType, ndim> in_array);
  DenseIndexArray(DenseIndexArraySingleInitTag, IndexType const& init_value);

  IndexType& operator[](IndexType const& index);
  IndexType const& operator[](IndexType const& index) const;
  IndexType get(IndexType const& index) const;
  IndexType const* raw() const;
  IndexSizeType packedSize() const;
  bool indexIsByteCopyable() const;
  DenseArraySizeType getSize() const;
  std::string toString() const;
  void foreach(ThisIndexType max, ApplyType fn) const;
  void foreach(ApplyType fn) const;
  UniqueIndexBitType uniqueBits() const;

  static ThisIndexType uniqueBitsToIndex(UniqueIndexBitType const& bits);

  bool operator==(DenseIndexArrayType const& other) const;
  bool operator<(DenseIndexArrayType const& other) const;

  // operator + and - for accessing neighboring indices
  DenseIndexArrayType operator+(DenseIndexArrayType const& other) const;
  DenseIndexArrayType operator-(DenseIndexArrayType const& other) const;

  // special accessors (x,y,z) enabled depending on the number of dimensions
  template <
    typename T = void, typename = typename std::enable_if<ndim >= 1, T>::type
  >
  IndexType x() const;

  template <
    typename T = void, typename = typename std::enable_if<ndim >= 2, T>::type
  >
  IndexType y() const;

  template <
    typename T = void, typename = typename std::enable_if<ndim >= 3, T>::type
  >
  IndexType z() const;

  template <typename IndexT, NumDimensionsType nd>
  friend std::ostream& operator<<(
    std::ostream& os, DenseIndexArray<IndexT,nd> const& idx
  );

private:
  std::array<IndexType, ndim> dims = {};
};

#if backend_check_enabled(detector)
  static_assert(
    vt::index::IndexTraits<DenseIndexArray<int, 10>>::is_index,
    "DenseIndexArray must follow the index concept"
  );
#endif

}}  // end namespace vt::index

#include "vt/topos/index/dense/dense_array.impl.h"

#endif /*INCLUDED_TOPOS_INDEX_DENSE_DENSE_ARRAY_H*/
