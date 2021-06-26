/*
//@HEADER
// *****************************************************************************
//
//                                null_atomic.h
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

#if !defined INCLUDED_VT_UTILS_ATOMIC_NULL_ATOMIC_H
#define INCLUDED_VT_UTILS_ATOMIC_NULL_ATOMIC_H

#include "vt/config.h"

#if backend_no_threading

// Needed for memory order
#include <atomic>

namespace vt { namespace util { namespace atomic {

static constexpr std::memory_order order_default = std::memory_order_seq_cst;

template <typename T>
struct AtomicNull {
  using OrderType = std::memory_order;

  AtomicNull(T&& in_value) : value_(std::forward<T>(in_value)) { }
  AtomicNull(T const& in_value) : value_(in_value) { }
  AtomicNull(AtomicNull const&) = delete;
  AtomicNull& operator=(AtomicNull const&) = delete;

  inline void add(T const in) { value_ += in; }
  inline void sub(T const in) { value_ -= in; }
  inline T fetch_add(T const in) { T tmp = value_; value_ += in; return tmp; }
  inline T add_fetch(T const in) { value_ += in; return value_; }
  inline T fetch_sub(T const in) { T tmp = value_; value_ -= in; return tmp; }
  inline T sub_fetch(T const in) { value_ -= in; return value_; }

  // pre-increment
  T operator++() { return fetch_add(1) + 1; }
  T operator--() { return fetch_sub(1) - 1; }
  // post-increment
  T operator++(int) { return fetch_add(1); }
  T operator--(int) { return fetch_sub(1); }
  // operators +=, -= (pre-increment type operation)
  T operator+=(T const& val) { return fetch_add(val) + val; }
  T operator-=(T const& val) { return fetch_sub(val) + val; }

  operator T() const { return load(); }
  inline T load(OrderType t = order_default) const { return value_; }
  inline void store(T in, OrderType t = order_default) { value_ = in; }
  T operator=(T in) { store(in); return in; }

  bool compare_exchange_strong(
    T& expected, T desired,
    OrderType st = order_default, OrderType sf = order_default
  ) {
    return value_ == expected ? value_ = desired, true : false;
  }

private:
  T value_;
};

}}} /* end namespace vt::util::atomic */

#endif /*backend_no_threading*/

#endif /*INCLUDED_VT_UTILS_ATOMIC_NULL_ATOMIC_H*/
