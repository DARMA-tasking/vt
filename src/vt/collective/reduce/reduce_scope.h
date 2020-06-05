/*
//@HEADER
// *****************************************************************************
//
//                                reduce_scope.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_SCOPE_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_SCOPE_H

#include "vt/collective/reduce/scoping/strong_types.h"
#include "vt/utils/adt/union.h"

namespace vt { namespace collective { namespace reduce { namespace detail {

/**
 * \struct ReduceScope
 *
 * \brief A unique scope for reduction operations, identified by the obj group
 * proxy, virtual proxy, group ID, or component ID.
 */
struct ReduceScope {
  using ValueType = vt::adt::SafeUnion<
    StrongObjGroup, StrongVrtProxy, StrongGroup, StrongCom, StrongUserID
  >;

  ReduceScope() = default;

  bool operator==(ReduceScope const& in) const {
    return in.l0_ == l0_;
  }
  bool operator!=(ReduceScope const& in) const {
    return !(this->operator==(in));
  }

  ValueType& get() { return l0_; }
  ValueType const& get() const { return l0_; }

  template <typename T, typename... Args>
  friend ReduceScope makeScope(Args&&... args);

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | l0_;
  }

  std::string str() const {
    if (l0_.is<StrongObjGroup>()) {
      return fmt::format("objgroup[{:x}]", l0_.get<StrongObjGroup>().get());
    } else if (l0_.is<StrongVrtProxy>()) {
      return fmt::format("vrtproxy[{:x}]", l0_.get<StrongVrtProxy>().get());
    } else if (l0_.is<StrongGroup>()) {
      return fmt::format("group[{:x}]", l0_.get<StrongGroup>().get());
    } else if (l0_.is<StrongCom>()) {
      return fmt::format("component[{}]", l0_.get<StrongCom>().get());
    } else if (l0_.is<StrongUserID>()) {
      return fmt::format("userID[{}]", l0_.get<StrongUserID>().get());
    } else {
      return "<unknown-type>";
    }
  }

private:
  ValueType l0_;
};

/**
 * \brief Create a new reduction scope
 *
 * \param[in] args constructor for reduction scope type
 *
 * \return the new scope
 */
template <typename T, typename... Args>
inline ReduceScope makeScope(Args&&... args);

/**
 * \brief Reduction stamp bits to identify a specific instance of a reduction.
 */
using ReduceStamp = vt::adt::SafeUnion<
  StrongTag, TagPair, StrongSeq, StrongUserID, StrongEpoch
>;

/**
 * \brief Stringize a \c ReduceStamp
 *
 * \param[in] stamp the reduction stamp
 *
 * \return a string
 */
inline std::string stringizeStamp(ReduceStamp const& stamp);

/**
 * \internal \struct ReduceIDImpl
 *
 * \brief Sent in all reduction messages to identify the scope and reduction
 * stamp so it gets merged with the right data.
 */
struct ReduceIDImpl {
  ReduceIDImpl() = default;
  ReduceIDImpl(ReduceIDImpl const&) = default;
  ReduceIDImpl(ReduceIDImpl&&) = default;

  ReduceIDImpl& operator=(ReduceIDImpl const& in) = default;

  ReduceIDImpl(ReduceStamp const& in_stamp, ReduceScope const& in_scope)
    : stamp_(in_stamp),
      scope_(in_scope)
  { }

  bool operator==(ReduceIDImpl const& in) const {
    return in.stamp_ == stamp_ and in.scope_ == scope_;
  }
  bool operator!=(ReduceIDImpl const& in) const {
    return !(this->operator==(in));
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | stamp_ | scope_;
  }

  ReduceStamp const& stamp() const { return stamp_; }
  ReduceScope const& scope() const { return scope_; }

protected:
  ReduceStamp stamp_;
  ReduceScope scope_;
};

static_assert(
  std::is_trivially_destructible<ReduceStamp>::value,
  "Must be trivially destructible"
);
static_assert(
  std::is_trivially_copyable<ReduceStamp>::value,
  "Must be trivially copyable"
);

static_assert(
  std::is_trivially_destructible<ReduceScope>::value,
  "Must be trivially destructible"
);
static_assert(
  std::is_trivially_copyable<ReduceScope>::value,
  "Must be trivially copyable"
);

static_assert(
  std::is_trivially_destructible<ReduceIDImpl>::value,
  "Must be trivially destructible"
);
static_assert(
  std::is_trivially_copyable<ReduceIDImpl>::value,
  "Must be trivially copyable"
);

/**
 * \internal \struct ReduceScopeHolder
 *
 * \brief Holds instances of reducers indexed by the reduction scope bits.
 */
template <typename T>
struct ReduceScopeHolder {
  using DefaultCreateFunction = std::function<T(detail::ReduceScope const&)>;

  explicit ReduceScopeHolder(DefaultCreateFunction in_default_creator)
    : default_creator_(in_default_creator)
  { }

  T& get(ReduceScope const& scope);

  template <typename U>
  T& getOnDemand(U&& scope);

  struct ObjGroupTag { };
  struct VrtProxyTag { };
  struct GroupTag { };
  struct ComponentTag { };
  struct UserIDTag { };

  T& get(ObjGroupTag, ObjGroupProxyType proxy);
  T& get(VrtProxyTag, VirtualProxyType proxy);
  T& get(GroupTag, GroupType group);
  T& get(ComponentTag, ComponentIDType component_id);
  T& get(UserIDTag, UserIDType user_id);

  void make(ObjGroupTag, ObjGroupProxyType proxy);
  void make(GroupTag, GroupType group, DefaultCreateFunction fn);

private:
  DefaultCreateFunction default_creator_ = nullptr;
  std::unordered_map<ReduceScope, T> scopes_;
};

}}}} /* end namespace vt::collective::reduce::detail */

namespace vt { namespace collective { namespace reduce {

using ReduceStamp = detail::ReduceStamp;
using UserIDType = detail::UserIDType;
using StrongUserID = detail::StrongUserID;
using StrongEpoch = detail::StrongEpoch;
using TagPair = detail::TagPair;

/**
 * \brief Create a new reduction stamp
 *
 * \param[in] args args to the stamp type constructor
 *
 * \return a new reduction stamp
 */
template <typename T, typename... Args>
ReduceStamp makeStamp(Args&&... args) {
  ReduceStamp stamp;
  stamp.init<T>(std::forward<Args>(args)...);
  return stamp;
}

}}} /* end namespace vt::collective::reduce */

namespace std {

template <>
struct hash<vt::collective::reduce::detail::ReduceScope> {
  size_t operator()(
    vt::collective::reduce::detail::ReduceScope const& in
  ) const {
    using ValueType =
      typename vt::collective::reduce::detail::ReduceScope::ValueType;
    return std::hash<ValueType>()(in.get());
  }
};

} /* end namespace std */

#include "vt/collective/reduce/reduce_scope.impl.h"

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_SCOPE_H*/
