/*
//@HEADER
// *****************************************************************************
//
//                               charmlb_msgs.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_CHARMLB_CHARMLB_MSGS_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_CHARMLB_CHARMLB_MSGS_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/balance/charmlb/charmlb_types.h"
#include "vt/messaging/message.h"
#include "vt/collective/reduce/operators/default_msg.h"

#include <unordered_map>
#include <cassert>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct CharmPayload : CharmLBTypes {
  using ObjLoadListType = BaseLB::ObjLoadListType;

  CharmPayload() = default;
  CharmPayload(ObjLoadListType const& in_objs, LoadType const& in_profile)
    : collected_objs_(in_objs)
  {
    auto const& this_node = theContext()->getNode();
    auto iter = load_profile_.find(this_node);
    auto end_iter = load_profile_.end();
    vtAssert(iter == end_iter, "Must not exist");
    load_profile_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(this_node),
      std::forward_as_tuple(in_profile)
    );
  }

  friend CharmPayload operator+(CharmPayload ld1, CharmPayload const& ld2) {
    auto& objs1 = ld1.collected_objs_;
    auto const& objs2 = ld2.collected_objs_;
    objs1.insert(objs1.end(), std::make_move_iterator(objs2.begin()),
                 std::make_move_iterator(objs2.end()));

    auto& load1 = ld1.load_profile_;
    auto const& load2 = ld2.load_profile_;
    for (auto&& elm : load2) {
      auto const& proc = elm.first;
      auto const& load = elm.second;
      vtAssert(load1.find(proc) == load1.end(), "Must not exist");
      load1[proc] = load;
    }
    return ld1;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | collected_objs_ | load_profile_;
  }

  ObjLoadListType const& getObjList() const { return collected_objs_; }
  ObjLoadListType&& getObjListMove() { return std::move(collected_objs_); }
  LoadProfileType const& getLoadProfile() const { return load_profile_; }
  LoadProfileType&& getLoadProfileMove() { return std::move(load_profile_); }

protected:
  LoadProfileType load_profile_;
  ObjLoadListType collected_objs_;
};

struct CharmCollectMsg : CharmLBTypes, collective::ReduceTMsg<CharmPayload> {
  using MessageParentType = collective::ReduceTMsg<CharmPayload>;
  using ObjLoadListType = BaseLB::ObjLoadListType;
  vt_msg_serialize_required(); // prev. serialize(1)

  CharmCollectMsg() = default;
  CharmCollectMsg(ObjLoadListType const& in_load, LoadType const& in_profile)
    : collective::ReduceTMsg<CharmPayload>(CharmPayload{in_load,in_profile})
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
  }

  ObjLoadListType const& getLoad() const {
    return collective::ReduceTMsg<CharmPayload>::getConstVal().getObjList();
  }

  ObjLoadListType&& getLoadMove() {
    return collective::ReduceTMsg<CharmPayload>::getVal().getObjListMove();
  }
};

struct CharmSendMsg : CharmLBTypes, vt::Message {
  using MessageParentType = vt::Message;
  vt_msg_serialize_required(); // vector

  CharmSendMsg() = default;
  explicit CharmSendMsg(std::vector<CharmLBTypes::ObjIDType> const& in)
    : transfer_(in)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | transfer_;
  }

  std::vector<CharmLBTypes::ObjIDType> transfer_;
};

struct CharmBcastMsg : CharmLBTypes, vt::Message {
  using MessageParentType = vt::Message;
  vt_msg_serialize_required(); // vector

  using DataType = std::vector<std::vector<CharmLBTypes::ObjIDType>>;

  CharmBcastMsg() = default;
  explicit CharmBcastMsg(DataType const& in)
    : transfer_(in)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | transfer_;
  }

  DataType transfer_;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_CHARMLB_CHARMLB_MSGS_H*/
