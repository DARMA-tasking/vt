/*
//@HEADER
// *****************************************************************************
//
//                             trace_user_event.cc
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

#include "vt/config.h"
#include "vt/trace/trace_common.h"
#include "vt/trace/trace_user_event.h"
#include "vt/utils/bits/bits_common.h"
#include "vt/messaging/active.h"

namespace vt { namespace trace {

UserEventIDType UserEventRegistry::createEvent(
  bool user, bool rooted, NodeType in_node, UserSpecEventIDType id,
  bool hash
) {
  constexpr NodeType const default_node = 0;
  NodeType const node = rooted ? in_node : default_node;
  UserEventIDType event = no_user_event_id;
  BitPackerType::boolSetField<eUserEventLayoutBits::Manu>(event, user);
  BitPackerType::boolSetField<eUserEventLayoutBits::Root>(event, rooted);
  BitPackerType::boolSetField<eUserEventLayoutBits::Hash>(event, hash);
  if (rooted and not hash) {
    BitPackerType::setField<eUserEventLayoutBits::Node, node_bits>(event, node);
  }
  BitPackerType::setField<eUserEventLayoutBits::ID, spec_bits>(event, id);
  return event;
}

/*static*/ void UserEventRegistry::newEventHan(NewUserEventMsg* msg) {
  vtAssert(theContext()->getNode() == 0, "Must be node 0");
  insertNewUserEvent(msg->id_, msg->name_);
}

UserEventIDType UserEventRegistry::hash(std::string const& in_event_name) {
  auto id_hash = static_cast<uint16_t>(std::hash<std::string>{}(in_event_name));
  id_hash = id_hash & 0x0FFF;
  auto ret = newEventImpl(false, false, in_event_name, id_hash, true);
  auto id = std::get<0>(ret);
  auto inserted = std::get<1>(ret);
  if (inserted) {
    auto const node  = theContext()->getNode();
    if (node != 0) {
      auto msg = makeMessage<NewUserEventMsg>(false, id, in_event_name);
      theMsg()->sendMsg<NewUserEventMsg,newEventHan>(0, msg.get());
    }
  }
  return id;
}

UserEventIDType UserEventRegistry::collective(std::string const& in_event_name) {
  auto ret = newEventImpl(false, false, in_event_name, cur_coll_event_++);
  return std::get<0>(ret);
}

UserEventIDType UserEventRegistry::rooted(std::string const& in_event_name) {
  auto ret = newEventImpl(false, true, in_event_name, cur_root_event_++);
  auto id = std::get<0>(ret);
  auto const node  = theContext()->getNode();
  if (node != 0) {
    auto msg = makeMessage<NewUserEventMsg>(false, id, in_event_name);
    theMsg()->sendMsg<NewUserEventMsg,newEventHan>(0, msg.get());
  }
  return id;
}

UserEventIDType UserEventRegistry::user(
  std::string const& in_event_name, UserSpecEventIDType seq
) {
  auto ret = newEventImpl(true, false, in_event_name, seq);
  auto id = std::get<0>(ret);
  auto const node  = theContext()->getNode();
  if (node != 0) {
    auto msg = makeMessage<NewUserEventMsg>(true, id, in_event_name);
    theMsg()->sendMsg<NewUserEventMsg,newEventHan>(0, msg.get());
  }
  return id;
}

std::tuple<UserEventIDType, bool> UserEventRegistry::newEventImpl(
  bool user, bool rooted, std::string const& in_event, UserSpecEventIDType id,
  bool hash
) {
  auto const node  = theContext()->getNode();
  auto const event = createEvent(user, rooted, node, id, hash);
  auto const inserted = insertEvent(event, in_event);
  return std::make_tuple(event, inserted);
}

bool UserEventRegistry::insertEvent(
  UserEventIDType event, std::string const& name
) {
  auto iter = user_event_.find(event);
  if (iter == user_event_.end()) {
    user_event_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(event),
      std::forward_as_tuple(name)
    );
    return true;
  } else {
    return false;
  }
}


}} /* end namespace vt::trace */
