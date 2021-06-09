/*
//@HEADER
// *****************************************************************************
//
//                                strong_type.h
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

#if !defined INCLUDED_VT_UTILS_STRONG_STRONG_TYPE_H
#define INCLUDED_VT_UTILS_STRONG_STRONG_TYPE_H

namespace vt { namespace util { namespace strong { namespace detail {

/**
 * \internal \struct Strong
 *
 * \brief Used to hoist weak C++ types (like \c vt::VirtualProxyType ) into
 * strongly typed values that have a unique type to enforce interfaces, inhibit
 * dangerous conversions, and provide more semantic control.
 *
 * All \c Strong types are arithmetic types, thus have operators defined on them
 */
template <typename T, T init_val, typename Tag>
struct Strong {
  using ThisType = Strong<T, init_val, Tag>;

  /// All \c Strong types are byte-copyable and arithmetic and thus don't need a
  /// serializer
  using isByteCopyable = std::true_type;

  /**
   * \brief Default construct with static initial value
   */
  constexpr Strong() = default;

  /**
   * \brief Construct with a explicit initial value
   *
   * \param[in] v the value
   */
  explicit constexpr Strong(T v) : v_(v) { }

  /**
   * \brief Equal operator
   *
   * \param[in] in the other value
   */
  bool operator==(ThisType const& in) const {
    return v_ == in.v_;
  }

  /**
   * \brief Non-equal operator
   *
   * \param[in] in the other value
   */
  bool operator!=(ThisType const& in) const {
    return v_ != in.v_;
  }

  /**
   * \brief Less-than operator
   *
   * \param[in] in the other value
   */
  bool operator<(ThisType const& in) const {
    return v_ < in.v_;
  }

  /**
   * \brief Less-than-equal operator
   *
   * \param[in] in the other value
   */
  bool operator<=(ThisType const& in) const {
    return v_ <= in.v_;
  }

  /**
   * \brief Greater-than operator
   *
   * \param[in] in the other value
   */
  bool operator>(ThisType const& in) const {
    return v_ > in.v_;
  }

  /**
   * \brief Greater-than-equal operator
   *
   * \param[in] in the other value
   */
  bool operator>=(ThisType const& in) const {
    return v_ >= in.v_;
  }

  /**
   * \brief Dereference the value as a reference
   *
   * \return reference to underlying value
   */
  T& operator*() { return v_; }

  /**
   * \brief Dereference the value as a const reference
   *
   * \return const reference to the underlying value
   */
  T const& operator*() const { return v_; }

  /**
   * \brief Get reference
   *
   * \return reference to underlying value
   */
  T& get() { return v_; }

  /**
   * \brief Get const reference
   *
   * \return const reference to underlying value
   */
  T const& get() const { return v_; }

private:
  T v_ = init_val;         /**< The underlying value held */
};

}}}} /* end namespace vt::util::strong::detail */

namespace std {

template <typename T, T init_val, typename Tag>
struct hash<vt::util::strong::detail::Strong<T, init_val, Tag>> {
  size_t operator()(
    vt::util::strong::detail::Strong<T, init_val, Tag> const& in
  ) const {
    return std::hash<T>()(in.get());
  }
};

} /* end namespace std */

namespace vt {

/// Type-alias for strong types
template <typename T, T init_val, typename Tag>
using Strong = util::strong::detail::Strong<T, init_val, Tag>;

} /* end namespace vt */

#endif /*INCLUDED_VT_UTILS_STRONG_STRONG_TYPE_H*/
