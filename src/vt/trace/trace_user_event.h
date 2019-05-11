/*
//@HEADER
// ************************************************************************
//
//                          trace_user_event.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_TRACE_TRACE_USER_EVENT_H
#define INCLUDED_VT_TRACE_TRACE_USER_EVENT_H

#include "vt/config.h"
#include "vt/trace/trace_common.h"
#include "vt/utils/bits/bits_common.h"
#include "vt/context/context.h"
#include "vt/messaging/message.h"

#include <string>
#include <unordered_map>
#include <cstdint>

namespace vt { namespace trace {

static constexpr BitCountType const user_bits = BitCounterType<UserEventIDType>::value;
static constexpr BitCountType const root_bits = 1;
static constexpr BitCountType const manu_bits = 1;
static constexpr BitCountType const node_bits = BitCounterType<NodeType>::value;
static constexpr BitCountType const spec_bits = user_bits - (root_bits + node_bits);

enum eUserEventLayoutBits {
  Manu  = 0,
  Root  = eUserEventLayoutBits::Manu + manu_bits,
  Node  = eUserEventLayoutBits::Root + root_bits,
  ID    = eUserEventLayoutBits::Node + node_bits
};

void insertNewUserEvent(UserEventIDType event, std::string const& name);

struct UserEventRegistry {

  struct NewUserEventMsg : vt::Message {
    NewUserEventMsg() = default;
    NewUserEventMsg(bool in_user, UserEventIDType in_id, std::string in_name)
      : user_(in_user), id_(in_id), name_(in_name)
    { }

    template <typename SerializerT>
    void serialize(SerializerT& s) {
      s | user_ | id_ | name_;
    }

    bool user_ = false;
    UserEventIDType id_ = 0;
    std::string name_;
  };

  static void newEventHan(NewUserEventMsg* msg);

  UserEventIDType
  createEvent(bool user, bool rooted, NodeType in_node, UserSpecEventIDType id);

  UserEventIDType collective(std::string const& in_event_name);
  UserEventIDType rooted(std::string const& in_event_name);
  UserEventIDType user(
    std::string const& in_event_name, UserSpecEventIDType id
  );

  friend void insertNewUserEvent(UserEventIDType event, std::string const& name);

private:
  UserEventIDType newEventImpl(
    bool user, bool rooted, std::string const& in_event, UserSpecEventIDType id
  );

  void insertEvent(UserEventIDType event, std::string const& name);

private:
  UserSpecEventIDType cur_root_event_                          = 1;
  UserSpecEventIDType cur_coll_event_                          = 1;
  std::unordered_map<UserEventIDType, std::string> user_event_ = {};
};

}} /* end namespace vt::trace */

#endif /*INCLUDED_VT_TRACE_TRACE_USER_EVENT_H*/
