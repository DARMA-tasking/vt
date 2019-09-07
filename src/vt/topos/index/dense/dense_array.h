/*
//@HEADER
// *****************************************************************************
//
//                                dense_array.h
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static struct DenseIndexArraySingleInitTag { } dense_single_value_tag { };
#pragma GCC diagnostic pop

template <typename IndexType, NumDimensionsType ndim>
struct DenseIndexArray : BaseIndex, serialization::ByteCopyTrait {
  using ThisIndexType = DenseIndexArray<IndexType, ndim>;
  using IndexSizeType = size_t;
  using ApplyType = std::function<void(ThisIndexType)>;
  using IsByteCopyable = serialization::ByteCopyTrait;
  using DenseIndexArrayType = DenseIndexArray<IndexType, ndim>;
  using DenseArraySizeType = uint64_t;
  using DenseIndexType = IndexType;
  using BuildIndexType = IndexType;

  DenseIndexArray() = default;
  DenseIndexArray(DenseIndexArray const&) = default;
  DenseIndexArray& operator=(DenseIndexArray const&) = default;
  DenseIndexArray(DenseIndexArray&&) = default;

  template <
    typename... Idxs,
    typename = typename std::enable_if<
      util::meta_type_eq<IndexType, typename std::decay<Idxs>::type...>::value and
      sizeof...(Idxs) == ndim
    >::type
  >
  explicit DenseIndexArray(Idxs&&... init);

  DenseIndexArray(std::array<IndexType, ndim> in_array);
  DenseIndexArray(DenseIndexArraySingleInitTag, IndexType const& init_value);

  NumDimensionsType ndims() const { return ndim; }

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
