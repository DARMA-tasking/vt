/*
//@HEADER
// *****************************************************************************
//
//                         context_vrt_internal_msgs.h
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

#if !defined INCLUDED_VRT_CONTEXT_CONTEXT_VRT_INTERNAL_MSGS_H
#define INCLUDED_VRT_CONTEXT_CONTEXT_VRT_INTERNAL_MSGS_H

#include "vt/config.h"
#include "vt/vrt/proxy/proxy_bits.h"
#include "vt/vrt/context/context_vrt_fwd.h"
#include "vt/messaging/message.h"

#include <array>

namespace vt { namespace vrt {

template <typename RemoteInfo, typename ArgsTuple, typename VirtualContextT>
struct VrtConstructMsg : Message {
  using MessageParentType = Message;
  vt_msg_serialize_if_needed_by_parent_or_type2(RemoteInfo, ArgsTuple);

  using VirtualContextType = VirtualContextT;
  using ArgsTupleType = ArgsTuple;

  RemoteInfo info;
  ArgsTuple tup;

  VrtConstructMsg() = default;

  VrtConstructMsg(ArgsTuple&& in_tup)
    : Message(), tup(std::forward<ArgsTuple>(in_tup))
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | info | tup;
  }
};

struct VirtualProxyRequestMsg : ShortMessage {
  using MessageParentType = ShortMessage;
  vt_msg_serialize_prohibited();

  NodeType request_node = uninitialized_destination;
  NodeType construct_node = uninitialized_destination;
  VirtualRequestIDType request_id = no_request_id;
  VirtualProxyType proxy = no_vrt_proxy;

  VirtualProxyRequestMsg(
    NodeType const& in_node, NodeType const& in_req_node,
    VirtualRequestIDType const& in_request_id,
    VirtualProxyType const& in_proxy = no_vrt_proxy
  )
    : ShortMessage(), request_node(in_req_node), construct_node(in_node),
      request_id(in_request_id), proxy(in_proxy)
  { }
};

}} /* end namespace vt::vrt */

#endif /*INCLUDED_VRT_CONTEXT/CONTEXT_VRT_INTERNAL_MSGS_H*/
