/*
//@HEADER
// ************************************************************************
//
//                          array.h
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

#if !defined INCLUDED_VT_FETCH_PAYLOAD_ARRAY_H
#define INCLUDED_VT_FETCH_PAYLOAD_ARRAY_H

#include "vt/config.h"
#include "vt/fetch/span.h"
#include "vt/fetch/traits/ptr.h"

#include <memory>

namespace vt { namespace fetch {

template <typename T>
struct FetchPayload<
  T,
  std::enable_if_t<
    PtrTraits<T>::arith and PtrTraits<T>::dims == 1 and not PtrTraits<T>::vec
  >
> {
  using ValueType = typename PtrTraits<T>::BaseType;
  using SpanType  = Span<ValueType>;

  FetchPayload(std::nullptr_t)
    : span_(std::make_shared<SpanType>(SpanUnitializedTag))
  { }
  FetchPayload(PayloadNewTagType, int64_t len)
    : alloc_(std::make_unique<ValueType[]>(len)),
      span_(std::make_shared<SpanType>(alloc_.get(),len))
  { }
  FetchPayload(PayloadRefTagType, std::shared_ptr<SpanType> in_span)
    : span_(in_span)
  { }
  FetchPayload(PayloadRefTagType, FetchPayload<T> const& in)
    : span_(in.span_)
  { }
  FetchPayload(PayloadMoveTagType, FetchPayload<T>&& in)
    : alloc_(std::move(alloc_)), span_(std::move(in.span_))
  { }
  FetchPayload(PayloadCopyTagType, std::shared_ptr<SpanType> in_span)
    : alloc_(std::make_unique<ValueType[]>(in_span->size())),
      span_(std::make_shared<SpanType>(alloc_.get(),in_span->size()))
  {
    vtAssertExpr(in_span->init());
    span_->copySpan(*in_span);
  }
  template <typename U>
  FetchPayload(PayloadCopyTagType, FetchPayload<U> const& in)
    : FetchPayload(PayloadCopyTag, in.span_)
  { }

  template  <typename U>
  void updateInternal(FetchPayload<U> const& in) {
    *span_ = *in.span_;
  }

  void allocate(int64_t len) {
    alloc_ = std::make_unique<ValueType[]>(len);
    span_->set(alloc_.get(),len);
  }

  void update(ValueType* in_data, int64_t len) {
    span_->set(in_data,len);
  }

  ValueType* getPtr() const {
    vtAssertExpr(span_->init());
    return span_->data();
  }

  ValueType* get() const {
    return getPtr();
  }

  int64_t size() const { return span_->size(); }

  bool pending() const { return not span_->init(); }
  bool ready() const { return not pending(); }

  template <typename U, typename V>
  friend struct FetchCtrl;
  template <typename U, typename V>
  friend struct FetchPayload;

private:
  std::unique_ptr<ValueType[]> alloc_ = nullptr;
  std::shared_ptr<SpanType>    span_  = nullptr;
};

}} /* end namespace vt::fetch */

#endif /*INCLUDED_VT_FETCH_PAYLOAD_ARRAY_H*/
