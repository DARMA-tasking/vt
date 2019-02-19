/*
//@HEADER
// ************************************************************************
//
//                          fetch_base.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_VRT_COLLECTION_FETCH_FETCH_H
#define INCLUDED_VT_VRT_COLLECTION_FETCH_FETCH_H

#include "vt/config.h"
#include "vt/vrt/collection/fetch/fetch_base.h"
#include "vt/vrt/collection/fetch/span.h"

#include <vector>
#include <cstdlib>

namespace vt { namespace vrt { namespace collection {

template <typename DataT, typename Enable=void>
struct FetchTraits;

template <typename DataT>
struct FetchTraits<
  DataT,
  typename std::enable_if_t<
    std::is_same<DataT, std::vector<typename DataT::value_type>>::value
  >
> : Span<typename DataT::value_type> {

  using ValueType = typename DataT::value_type;
  using SpanType  = Span<ValueType>;

  explicit FetchTraits(std::vector<ValueType>& in)
    : SpanType(&in[0],in.size()),
      ptr_(&in),
      owning_(false)
  { }
  FetchTraits() : Span<ValueType>(SpanUnitializedTag) { }

  void set(std::vector<ValueType>& in) {
    auto const& init = SpanType::init();
    if (init) {
      vtAsserExpr(in.size() == SpanType::size(), "Sizes must be identical");
      std::memcpy(SpanType::get(), &in[0], in.size()*sizeof(ValueType));
    } else {
      ptr_ = &in;
      SpanType::set(&in[0], in.size());
    }
  }

  void set(std::vector<ValueType>&& in) {
    auto const& init = SpanType::init();
    if (init) {
      *ptr_ = std::move(in);
    } else {
      ptr_ = new std::vector<ValueType>(std::move(in));
      owning_ = true;
    }
    SpanType::set(&(*ptr_)[0], ptr_->size());
  }

  std::vector<ValueType> const& operator*() { return *ptr_; }

  virtual ~FetchTraits() {
    if (ptr_ and owning_) {
      delete ptr_;
      ptr_ = nullptr;
      owning_ = false;
    }
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    SpanType::serialize(s);
  }

private:
  std::vector<ValueType>* ptr_ = nullptr;
  bool owning_ = false;
};


template <typename T>
struct FetchTraits<
  T,
  typename std::enable_if_t<std::is_arithmetic<T>::value>
> : Span<T> {

  FetchTraits(T* in, int64_t in_len)
    : Span<T>(in,in_len)
  { }
  FetchTraits() : Span<T>(SpanUnitializedTag) { }

  void set(T* in, int64_t len) {
    auto const& init = Span<T>::init();
    if (init) {
      std::memcpy(Span<T>::get(), in, len*sizeof(T));
    } else {
      Span<T>::set(in, len);
    }
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    Span<T>::serialize(s);
  }
};

template <typename T>
using Fetch = FetchTraits<T>;

// template <typename T>
// struct Fetch : FetchTraits<T> {
//   Fetch() = delete;
//   Fetch(Fetch const&) = default;
//   Fetch(Fetch&&) = default;
//   Fetch& operator=(Fetch const&) = default;
//   virtual ~Fetch() = default;
// };

}}} /* end namespace vt::vrt::collection */

namespace vt {

template <typename T>
using Fetch = vrt::collection::Fetch<T>;

} /* end namespace vt */

#endif /*INCLUDED_VT_VRT_COLLECTION_FETCH_FETCH_H*/
