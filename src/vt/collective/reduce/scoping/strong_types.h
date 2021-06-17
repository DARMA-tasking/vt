/*
//@HEADER
// *****************************************************************************
//
//                                strong_types.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_SCOPING_STRONG_TYPES_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_SCOPING_STRONG_TYPES_H

#include "vt/configs/types/types_type.h"
#include "vt/configs/types/types_sentinels.h"
#include "vt/collective/reduce/scoping/strong.h"

namespace vt { namespace collective { namespace reduce { namespace detail {

using UserIDType = uint64_t;

namespace tags {

struct TagTag {};
struct SeqTag {};
struct ObjGroupTag {};
struct VrtProxyTag {};
struct GroupTag {};
struct ComponentTag {};
struct UserIDTag {};
struct EpochTag {};

} /* end namespace tags */

using StrongTag      = Strong<TagType,           no_tag,       tags::TagTag>;
using StrongSeq      = Strong<SequentialIDType,  no_seq_id,    tags::SeqTag>;
using StrongObjGroup = Strong<ObjGroupProxyType, no_obj_group, tags::ObjGroupTag>;
using StrongVrtProxy = Strong<VirtualProxyType,  no_vrt_proxy, tags::VrtProxyTag>;
using StrongGroup    = Strong<GroupType,         no_group,     tags::GroupTag>;
using StrongCom      = Strong<ComponentIDType,   u32empty,     tags::ComponentTag>;
using StrongUserID   = Strong<UserIDType,        u64empty,     tags::UserIDTag>;
using StrongEpoch    = Strong<EpochType,         no_epoch,     tags::EpochTag>;

/**
 * \struct TagPair
 *
 * \brief Holds a pair of tags, which can be used to identify a reduction
 * instance
 */
struct TagPair {
  TagPair() = default;
  TagPair(TagType in_t1, TagType in_t2)
    : t1_(in_t1), t2_(in_t2)
  { }

  bool operator==(TagPair const& in) const {
    return in.first() == first() and in.second() == second();
  }

  bool operator!=(TagPair const& in) const {
    return !(this->operator==(in));
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | t1_ | t2_;
  }

  TagType first() const { return t1_; }
  TagType second() const { return t2_; }

private:
  TagType t1_ = no_tag;
  TagType t2_ = no_tag;
};

}}}} /* end namespace vt::collective::reduce::detail */

namespace std {

template <>
struct hash<vt::collective::reduce::detail::TagPair> {
  size_t operator()(
    vt::collective::reduce::detail::TagPair const& in
  ) const {
    return std::hash<vt::TagType>()(in.first()) ^
           std::hash<vt::TagType>()(in.second());
  }
};

} /* end namespace std */

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_SCOPING_STRONG_TYPES_H*/
