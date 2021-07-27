/*
//@HEADER
// *****************************************************************************
//
//                               system_create.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_MESSAGES_SYSTEM_CREATE_H
#define INCLUDED_VT_VRT_COLLECTION_MESSAGES_SYSTEM_CREATE_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vt/vrt/collection/types/headers.h"
#include "vt/messaging/message.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/vrt/proxy/collection_proxy.h"

namespace vt { namespace vrt { namespace collection {

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

struct CollectionStampMsg : vt::collective::ReduceTMsg<SequentialIDType> {
  using MessageParentType = vt::collective::ReduceTMsg<SequentialIDType>;
  vt_msg_serialize_prohibited();

  CollectionStampMsg() = default;
  CollectionStampMsg(
    VirtualProxyType const in_proxy, SequentialIDType const in_val
  ) : vt::collective::ReduceTMsg<SequentialIDType>(in_val),
      proxy_(in_proxy)
  { }

  VirtualProxyType proxy_ = no_vrt_proxy;
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

template <typename ColT>
struct InsertMsg : ::vt::Message {
  using IndexType = typename ColT::IndexType;

  using MessageParentType = ::vt::Message;
  vt_msg_serialize_prohibited();

  InsertMsg() = default;

  InsertMsg(
    CollectionProxy<ColT> in_proxy, IndexType in_idx, NodeType in_construct_node,
    NodeType in_home_node, EpochType in_insert_epoch
  ) : proxy_(in_proxy),
      idx_(in_idx),
      construct_node_(in_construct_node),
      home_node_(in_home_node),
      insert_epoch_(in_insert_epoch)
  { }

  CollectionProxy<ColT> proxy_ = {};
  IndexType idx_ = {};
  NodeType construct_node_ = uninitialized_destination;
  NodeType home_node_ = uninitialized_destination;
  EpochType insert_epoch_ = no_epoch;
  bool pinged_ = false;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_MESSAGES_SYSTEM_CREATE_H*/
