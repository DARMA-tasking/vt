/*
//@HEADER
// *****************************************************************************
//
//                                 epoch_type.h
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

#if !defined INCLUDED_VT_EPOCH_EPOCH_TYPE_H
#define INCLUDED_VT_EPOCH_EPOCH_TYPE_H

#include "vt/utils/strong/strong_type.h"
#include "vt/epoch/epoch_impl_type.h"

#include "vt/cmake_config.h"

namespace vt { namespace epoch {

struct EpochType : Strong<
  detail::EpochImplType, detail::no_epoch_impl, detail::EpochImplTag
> {
  /// Base class for strong type
  using BaseType = Strong<
    detail::EpochImplType, detail::no_epoch_impl, detail::EpochImplTag
  >;

  /// \c EpochType is always byte-copyable
  using isByteCopyable = std::true_type;

  /// The underlying type for an epoch
  using ImplType = detail::EpochImplType;

  EpochType() = default;

  /**
   * \brief Construct with a particular underlying value
   *
   * \param[in] in the value
   */
  explicit constexpr EpochType(detail::EpochImplType in)
    : BaseType(in)
  { }

  /**
   * \brief Constructor from base class for operator up-conversion
   *
   * \param[in] in the underlying strong value
   */
  explicit EpochType(BaseType const& in)
    : BaseType(in)
  { }
};

/**
 * \internal \brief Make an epoch that is zero, helper function for system epoch
 * manipulation
 *
 * \return zero'ed epoch
 */
constexpr inline EpochType makeEpochZero() {
  return EpochType{static_cast<EpochType::ImplType>(0ull)};
}

inline auto format_as(EpochType e) { return *e; }

}} /* end namespace vt::epoch */

namespace std {

/// Hash function for \c EpochType
template <>
struct hash<vt::epoch::EpochType> {
  size_t operator()(vt::epoch::EpochType const& in) const {
    return std::hash<vt::epoch::EpochType::ImplType>()(in.get());
  }
};

} /* end namespace std */

namespace vt {

/// The strong epoch type for holding a epoch for termination detection
using EpochType = epoch::EpochType;

/// The sentinel value for a empty epoch
static constexpr EpochType const no_epoch = {};

} /* end namespace vt */

#endif /*INCLUDED_VT_EPOCH_EPOCH_TYPE_H*/
