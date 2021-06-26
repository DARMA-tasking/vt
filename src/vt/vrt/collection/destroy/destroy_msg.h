/*
//@HEADER
// *****************************************************************************
//
//                                destroy_msg.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_DESTROY_DESTROY_MSG_H
#define INCLUDED_VT_VRT_COLLECTION_DESTROY_DESTROY_MSG_H

#include "vt/config.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/messaging/message.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct DestroyMsg final : ::vt::Message {
  using MessageParentType = ::vt::Message;
  using CollectionProxyType = CollectionProxy<ColT,IndexT>;
  vt_msg_serialize_if_needed_by_parent_or_type1(CollectionProxyType);

  DestroyMsg() = default;
  DestroyMsg(
    CollectionProxyType const& in_proxy,
    NodeType const& in_from
  ) : proxy_(in_proxy), from_(in_from)
  { }

  CollectionProxyType getProxy() const { return proxy_; }
  NodeType getFromNode() const { return from_; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | proxy_ | from_;
  }

private:
  CollectionProxyType proxy_;
  NodeType from_ = uninitialized_destination;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_DESTROY_DESTROY_MSG_H*/
