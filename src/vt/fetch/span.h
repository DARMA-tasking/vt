/*
//@HEADER
// ************************************************************************
//
//                             span.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_FETCH_SPAN_H
#define INCLUDED_VT_VRT_COLLECTION_FETCH_SPAN_H

#include "vt/config.h"
#include "vt/fetch/fetch_base.h"

#if HAS_SERIALIZATION_LIBRARY
  #define HAS_DETECTION_COMPONENT 1
  #include "serialization_library_headers.h"
  #include "traits/serializable_traits.h"
#endif

namespace vt { namespace fetch {

static struct SpanUnitializedTagType { } SpanUnitializedTag { };

template <typename T>
struct Span : FetchBase {
  Span() = delete;
  explicit Span(SpanUnitializedTagType)
    : init_(false), data_(nullptr), len_(0)
  { }
  Span(T* in_data, int64_t in_len)
    : init_(true), data_(in_data), len_(in_len)
  { }
  Span(Span const&) = default;
  Span(Span&&) = default;
  Span& operator=(Span const&) = default;

public:
  int64_t size() const { return len_; }
  T* data() const { return data_; }
  T& operator[](int64_t u) const { return data_[u]; }
  bool init() const { return init_; }

  void set(T* data, int64_t len) {
    data_ = data;
    len_ = len;
    init_ = true;
  }

  void copySpan(Span<T> span) {
    vtAssertExpr(span.len_ == len_);
    for (auto i = 0; i < len_; i++) {
      data_[i] = span.data_[i];
    }
  }

public:
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | init_ | len_;
    serdes::serializeArray(s, data_, len_);
  }

private:
  bool init_ = false;
  T* data_ = nullptr;
  int64_t len_ = 0;
};

}} /* end namespace vt::fetch */

#endif /*INCLUDED_VT_VRT_COLLECTION_FETCH_SPAN_H*/
