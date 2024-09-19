/*
//@HEADER
// *****************************************************************************
//
//                            strong_integral_set.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_TERMINATION_INTERVAL_STRONG_INTEGRAL_SET_H
#define INCLUDED_VT_TERMINATION_INTERVAL_STRONG_INTEGRAL_SET_H

#include "vt/termination/interval/integral_set.h"

namespace vt { namespace term { namespace interval {

template <typename T>
struct StrongIntegralSet {
  using ImplType = typename T::ImplType;
  using SetType = IntegralSet<ImplType>;

  StrongIntegralSet() = default;
  explicit StrongIntegralSet(ImplType in)
    : impl_(in)
  { }

  template <
    typename U,
    typename = std::enable_if_t<
      std::is_same<
        std::remove_const_t<std::remove_reference_t<U>>, T
      >::value
    >
  >
  auto insert(U&& val) {
    return impl_.insert(*std::forward<U>(val));
  }

  template <
    typename U,
    typename = std::enable_if_t<
      std::is_same<
        std::remove_const_t<std::remove_reference_t<U>>, T
      >::value
    >
  >
  auto insert(typename SetType::IteratorType it, U&& val) {
    return impl_.insert(it, *std::forward<U>(val));
  }

  template <
    typename IntervalU,
    typename = std::enable_if_t<
      std::is_same<
        std::remove_const_t<std::remove_reference_t<IntervalU>>,
        typename SetType::IntervalType
      >::value
    >
  >
  auto insertInterval(IntervalU&& i) {
    return impl_.insertInterval(std::forward<IntervalU>(i));
  }

  template <
    typename IntervalU,
    typename = std::enable_if_t<
      std::is_same<
        std::remove_const_t<std::remove_reference_t<IntervalU>>,
        typename SetType::IntervalType
      >::value
    >
  >
  auto insertInterval(typename SetType::IteratorType it, IntervalU&& i) {
    return impl_.insertInterval(it, std::forward<IntervalU>(i));
  }

  auto erase(T const& val) { impl_.erase(*val); }
  auto contains(T const& val) const { return impl_.contains(*val); }
  auto exists(T const& val) const { return impl_.exists(*val); }
  auto clear() { impl_.clear(); }
  auto range() const { return T{impl_.range()}; }
  auto lower() const { return T{impl_.lower()}; }
  auto upper() const { return T{impl_.upper()}; }
  auto empty() const { return impl_.empty(); }
  auto size() const { return impl_.size(); }
  auto compressedSize() const { return impl_.compressedSize(); }
  auto compression() const { return impl_.compression(); }
  auto dumpState() const { impl_.dumpState(); }

private:
  SetType impl_;
};

}}} /* end namespace vt::term::interval */

namespace vt {

template <typename T>
using StrongIntegralSet = term::interval::StrongIntegralSet<T>;

} /* end namespace vt */


#endif /*INCLUDED_VT_TERMINATION_INTERVAL_STRONG_INTEGRAL_SET_H*/
