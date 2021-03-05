/*
//@HEADER
// *****************************************************************************
//
//                               system_create.h
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

#if !defined INCLUDED_VRT_COLLECTION_MESSAGES_SYSTEM_CREATE_H
#define INCLUDED_VRT_COLLECTION_MESSAGES_SYSTEM_CREATE_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vt/vrt/collection/types/headers.h"
#include "vt/messaging/message.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/vrt/proxy/collection_proxy.h"

namespace vt { namespace vrt { namespace collection {

template <
  typename RemoteInfo, typename ArgsTuple, typename CollectionT, typename IndexT
>
struct CollectionCreateMsg : ::vt::Message {
  using MessageParentType = ::vt::Message;
  vt_msg_serialize_if_needed_by_parent_or_type2(RemoteInfo, ArgsTuple);

  using CollectionType = CollectionT;
  using IndexType = IndexT;
  using ArgsTupleType = ArgsTuple;

  RemoteInfo info;
  ArgsTuple tup;
  HandlerType map;

  CollectionCreateMsg() = default;
  CollectionCreateMsg(
    HandlerType const in_han, ArgsTuple&& in_tup
  ) : ::vt::Message(), tup(std::forward<ArgsTuple>(in_tup)), map(in_han)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | info | tup | map;
  }
};

struct CollectionConsMsg : ::vt::collective::ReduceNoneMsg {
  using MessageParentType = ::vt::collective::ReduceNoneMsg;
  vt_msg_serialize_prohibited();

  CollectionConsMsg() = default;
  explicit CollectionConsMsg(VirtualProxyType const& in_proxy)
    : proxy(in_proxy)
  { }

  VirtualProxyType getProxy() const { return proxy; }

  VirtualProxyType proxy = {};
};

struct CollectionGroupMsg : CollectionConsMsg {
  using MessageParentType = CollectionConsMsg;
  vt_msg_serialize_prohibited();

  CollectionGroupMsg() = default;
  CollectionGroupMsg(
    VirtualProxyType const& in_proxy, GroupType const& in_group
  ) : CollectionConsMsg(in_proxy), group_(in_group)
  { }

  GroupType getGroup() const { return group_; }

private:
  GroupType group_ = no_group;
};

struct FinishedUpdateMsg : ::vt::collective::reduce::ReduceMsg {
  using MessageParentType = ::vt::collective::reduce::ReduceMsg;
  vt_msg_serialize_prohibited();

  FinishedUpdateMsg() = default;
  explicit FinishedUpdateMsg(VirtualProxyType const& in_proxy)
    : proxy_(in_proxy)
  { }

  VirtualProxyType proxy_ = {};
};

template <typename ColT, typename IndexT>
struct InsertMsg : ::vt::Message {
  using MessageParentType = ::vt::Message;
  vt_msg_serialize_prohibited();

  InsertMsg() = default;

  InsertMsg(
    CollectionProxy<ColT,IndexT> in_proxy,
    IndexT in_max, IndexT in_idx, NodeType in_construct_node,
    NodeType in_home_node, EpochType in_epoch, EpochType in_g_epoch
  ) : proxy_(in_proxy), max_(in_max), idx_(in_idx),
      construct_node_(in_construct_node), home_node_(in_home_node),
      epoch_(in_epoch), g_epoch_(in_g_epoch)
  { }

  CollectionProxy<ColT,IndexT> proxy_ = {};
  IndexT max_ = {}, idx_ = {};
  NodeType construct_node_ = uninitialized_destination;
  NodeType home_node_ = uninitialized_destination;
  EpochType epoch_ = no_epoch;
  EpochType g_epoch_ = no_epoch;
  bool pinged_ = false;
};

template <typename ColT, typename IndexT>
struct DoneInsertMsg : ::vt::Message {
  using MessageParentType = ::vt::Message;
  vt_msg_serialize_prohibited();

  DoneInsertMsg() = default;

  DoneInsertMsg(
    CollectionProxy<ColT,IndexT> in_proxy,
    NodeType const& in_action_node = uninitialized_destination
  ) : action_node_(in_action_node), proxy_(in_proxy)
  { }

  NodeType action_node_ = uninitialized_destination;
  CollectionProxy<ColT,IndexT> proxy_ = {};
};

template <typename ColT, typename IndexT>
struct ActInsertMsg : ::vt::Message {
  using MessageParentType = ::vt::Message;
  vt_msg_serialize_prohibited();

  ActInsertMsg() = default;

  explicit ActInsertMsg(CollectionProxy<ColT,IndexT> in_proxy)
    : proxy_(in_proxy)
  { }

  CollectionProxy<ColT,IndexT> proxy_ = {};
};

template <typename ColT, typename IndexT>
struct UpdateInsertMsg : ::vt::Message {
  using MessageParentType = ::vt::Message;
  vt_msg_serialize_prohibited();

  UpdateInsertMsg() = default;

  UpdateInsertMsg(
    CollectionProxy<ColT,IndexT> in_proxy, EpochType const& in_epoch
  ) : proxy_(in_proxy), epoch_(in_epoch)
  { }

  CollectionProxy<ColT,IndexT> proxy_ = {};
  EpochType epoch_ = no_epoch;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_SYSTEM_CREATE_H*/
