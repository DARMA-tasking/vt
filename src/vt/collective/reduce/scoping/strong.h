/*
//@HEADER
// *****************************************************************************
//
//                                   strong.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_SCOPING_STRONG_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_SCOPING_STRONG_H

namespace vt { namespace collective { namespace reduce { namespace detail {

/**
 * \internal \struct Strong
 *
 * \brief Used to hoist VT types (like \c vt::VirtualProxyType ) into strongly
 * typed values that have a unique type.
 */
template <typename T, T initial_value, typename Tag>
struct Strong {
  Strong() = default;
  explicit Strong(T v) : v_(v) { }
  operator T() { return v_; }
  operator T const() { return v_; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | v_;
  }

  bool operator==(Strong<T, initial_value, Tag> const& in) const {
    return v_ == in.v_;
  }

  bool operator!=(Strong<T, initial_value, Tag> const& in) const {
    return v_ != in.v_;
  }

  Strong<T,initial_value,Tag>& operator++() {
    v_++;
    return *this;
  }

  T& operator*() { return v_; }
  T const& operator*() const { return v_; }

  T& get() { return v_; }
  T const& get() const { return v_; }

private:
  T v_ = initial_value;
};

}}}} /* end namespace vt::collective::reduce::detail */

namespace std {

template <typename T, T initial_value, typename Tag>
struct hash<vt::collective::reduce::detail::Strong<T, initial_value, Tag>> {
  size_t operator()(
    vt::collective::reduce::detail::Strong<T, initial_value, Tag> const& in
  ) const {
    return std::hash<T>()(in.get());
  }
};

} /* end namespace std */

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_SCOPING_STRONG_H*/
