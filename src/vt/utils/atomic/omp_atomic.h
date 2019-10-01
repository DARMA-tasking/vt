/*
//@HEADER
// *****************************************************************************
//
//                                 omp_atomic.h
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

#if !defined INCLUDED_UTILS_ATOMIC_OMP_ATOMIC_H
#define INCLUDED_UTILS_ATOMIC_OMP_ATOMIC_H

#include "vt/config.h"

#if backend_check_enabled(openmp)

#include <atomic>
#include <omp.h>

namespace vt { namespace util { namespace atomic {

template <typename T>
struct AtomicOMP {
  AtomicOMP(T&& in_value) : value_(std::forward<T>(in_value)) { }
  AtomicOMP(T const& in_value) : value_(in_value) { }
  AtomicOMP(AtomicOMP const&) = delete;
  AtomicOMP& operator=(AtomicOMP const&) = delete;

  void add(T const in) {
    #pragma omp atomic update
    value_ += in;
  }

  void sub(T const in) {
    #pragma omp atomic update
    value_ -= in;
  }

  T fetch_add(T const in) {
    T tmp;
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-value"
    #pragma omp atomic capture
    {
      tmp = value_;
      value_ += in;
    }
    #pragma GCC diagnostic pop
    return tmp;
  }

  T add_fetch(T const in) {
    T tmp;
    #pragma omp atomic capture
    {
      value_ += in;
      tmp = value_;
    }
    return tmp;
  }

  T fetch_sub(T const in) {
    T tmp;
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-value"
    #pragma omp atomic capture
    {
      tmp = value_;
      value_ -= in;
    }
    #pragma GCC diagnostic pop
    return tmp;
  }

  T sub_fetch(T const in) {
    T tmp;
    #pragma omp atomic capture
    {
      value_ -= in;
      tmp = value_;
    }
    return tmp;
  }

  // pre-increment
  T operator++() { return fetch_add(1) + 1; }
  T operator--() { return fetch_sub(1) - 1; }
  // post-increment
  T operator++(int) { return fetch_add(1); }
  T operator--(int) { return fetch_sub(1); }
  // operators +=, -= (pre-increment type operation)
  T operator+=(T const& val) { return fetch_add(val) + val; }
  T operator-=(T const& val) { return fetch_sub(val) + val; }

  operator T() const {
    return load();
  }

  T load(std::memory_order order = std::memory_order_seq_cst) const {
    T tmp;
    #pragma omp atomic read
    tmp = value_;
    return tmp;
  }

  void store(T in, std::memory_order order = std::memory_order_seq_cst) {
    #pragma omp atomic write
    value_ = in;
  }

  T operator=(T in) {
    store(in);
    return in;
  }

  bool compare_exchange_strong(
    T& expected, T desired,
    std::memory_order success = std::memory_order_seq_cst,
    std::memory_order failure = std::memory_order_seq_cst
  ) {
    bool cas_succeed = false;
    #pragma omp critical
    {
      if (value_ == expected) {
        value_ = desired;
        cas_succeed = true;
      }
    }
    return cas_succeed;
  }

private:
  T value_;
};

}}} /* end namespace vt::util::atomic */

#endif /*backend_check_enabled(openmp)*/

#endif /*INCLUDED_UTILS_ATOMIC_OMP_ATOMIC_H*/
