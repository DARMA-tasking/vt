/*
//@HEADER
// *****************************************************************************
//
//                                  observer.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_UTILS_PTR_OBSERVER_H
#define INCLUDED_VT_UTILS_PTR_OBSERVER_H

#include "vt/config.h"

namespace vt { namespace util { namespace ptr {

/**
 * \struct ObserverPtr
 *
 * \brief A simple wrapper over a raw pointer that indicates it is held as an
 * observer that does not own or control the lifetime.
 */
template <typename T>
struct ObserverPtr final {

  explicit ObserverPtr(T* in_p)
    : p_(in_p)
  { }

  ObserverPtr(std::nullptr_t) { }
  ObserverPtr() = default;
  ObserverPtr(ObserverPtr const&) = default;
  ObserverPtr(ObserverPtr&&) = default;

  ObserverPtr<T>& operator=(std::nullptr_t) {
    p_ = nullptr;
    return *this;
  }

  ObserverPtr<T>& operator=(T* in) {
    p_ = in;
    return *this;
  }

  ObserverPtr<T>& operator=(ObserverPtr<T> const& in) {
    p_ = in.p_;
    return *this;
  }

  bool operator==(T* n) const { return p_ == n; }
  bool operator!=(T* n) const { return p_ != n; }
  bool operator==(ObserverPtr<T> const& n) const { return p_ == n.p_; }
  bool operator!=(ObserverPtr<T> const& n) const { return p_ != n.p_; }
  bool operator==(std::nullptr_t) const { return p_ == nullptr; }
  bool operator!=(std::nullptr_t) const { return p_ != nullptr; }

  operator T*() const { return p_; }
  T* operator*() const { return p_; }
  T* operator->() const { return p_; }
  T* get() const { return p_; }

  void reset() { p_ = nullptr; }

private:
  T* p_ = nullptr;
};

}}} /* end namespace vt::util::ptr */

namespace vt { namespace util {

template <typename T>
using ObserverPtr = ptr::ObserverPtr<T>;

}} /* end namespace vt::util */

#endif /*INCLUDED_VT_UTILS_PTR_OBSERVER_H*/
