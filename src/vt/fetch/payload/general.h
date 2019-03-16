/*
//@HEADER
// ************************************************************************
//
//                          general.h
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

#if !defined INCLUDED_VT_FETCH_PAYLOAD_GENERAL_H
#define INCLUDED_VT_FETCH_PAYLOAD_GENERAL_H

#include "vt/config.h"
#include "vt/fetch/traits/ptr.h"

namespace vt { namespace fetch {

template <typename T>
struct FetchPayload<
  T, std::enable_if_t<not (PtrTraits<T>::arith and PtrTraits<T>::dims == 1)>
> {

  FetchPayload(std::nullptr_t)
    : impl_(std::make_shared<T*>(nullptr))
  { }
  template <typename... Args>
  FetchPayload(PayloadNewTagType, Args&&... args)
    : init_(true),
      alloc_(std::make_unique<T>(std::forward<Args>(args)...)),
      impl_(std::make_shared<T*>(alloc_.get()))
  { }
  FetchPayload(PayloadRefTagType, FetchPayload<T> const& in)
    : init_(in.init_), impl_(in.impl_)
  { }
  FetchPayload(PayloadMoveTagType, FetchPayload<T>&& in)
    : init_(std::move(in.init_)), alloc_(std::move(alloc_)),
      impl_(std::move(in.impl_))
  { }
  template <typename U>
  FetchPayload(PayloadCopyTagType, FetchPayload<U> const& in)
    : init_(true),
      alloc_(std::make_unique<T>(*in.impl_)),
      impl_(std::make_shared<T*>(alloc_.get()))
  {
    vtAssertExpr(in.init_);
  }

  template <typename... Args>
  void allocate(Args&&... args) {
    alloc_ = std::make_unique<T>(std::forward<Args>(args)...);
    *impl_ = alloc_.get();
    init_ = true;
  }

  template  <typename U>
  void updateInternal(FetchPayload<U> const& in) {
    *impl_ = *in.impl_;
    init_ = true;
  }

  void update(T&& in) {
    alloc_ = std::make_unique<T>(std::move(in));
    *impl_ = alloc_.get();
    init_ = true;
  }

  void update(T const& in) {
    alloc_ = std::make_unique<T>(in);
    *impl_ = alloc_.get();
    init_ = true;
  }

  void update(T* const ptr) {
    *impl_ = ptr;
    init_ = true;
  }

  T* get() const {
    vtAssertExpr(init_);
    return *(impl_.get());
  }

  bool pending() const { return not init_; }
  bool ready() const { return not pending(); }

  template <typename U, typename V>
  friend struct FetchCtrl;
  template <typename U, typename V>
  friend struct FetchPayload;

private:
  bool init_                 = false;
  std::unique_ptr<T>  alloc_ = nullptr;
  std::shared_ptr<T*> impl_  = nullptr;
};

}} /* end namespace vt::fetch */

#endif /*INCLUDED_VT_FETCH_PAYLOAD_GENERAL_H*/
