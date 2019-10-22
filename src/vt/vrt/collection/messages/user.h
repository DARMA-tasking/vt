/*
//@HEADER
// *****************************************************************************
//
//                                    user.h
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

#if !defined INCLUDED_VRT_COLLECTION_MESSAGES_USER_H
#define INCLUDED_VRT_COLLECTION_MESSAGES_USER_H

#include "vt/config.h"
#include "vt/topos/location/message/msg.h"
#include "vt/messaging/message.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/collection/proxy.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/lb_comm.h"

#include <type_traits>

namespace vt { namespace vrt { namespace collection {

template <typename MessageT, typename ColT>
using RoutedMessageType = LocationRoutedMsg<
  ::vt::vrt::collection::IndexOnlyTypedProxy<typename ColT::IndexType>, MessageT
>;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static struct ColMsgWrapTagType { } ColMsgWrapTag { };
#pragma GCC diagnostic pop

template <typename ColT, typename BaseMsgT = ::vt::Message>
struct CollectionMessage :
  RoutedMessageType<BaseMsgT, ColT>, ColT::IndexType::IsByteCopyable
{
  /*
   *. Type aliases for surrounding system => used to deduce during sends
  */
  using CollectionType = ColT;
  using IndexType = typename ColT::IndexType;
  using IsCollectionMessage = std::true_type;
  using UserMsgType = void;

  CollectionMessage() = default;

  explicit CollectionMessage(ColMsgWrapTagType)
    : is_wrap_(true)
  { }

  void setVrtHandler(HandlerType const& in_handler);
  HandlerType getVrtHandler() const;

  // The variable `to_proxy_' manages the intended target of the
  // `CollectionMessage'
  VirtualElmProxyType<ColT, IndexType> getProxy() const;
  void setProxy(VirtualElmProxyType<ColT, IndexType> const& in_proxy);

  VirtualProxyType getBcastProxy() const;
  void setBcastProxy(VirtualProxyType const& in_proxy);

  EpochType getBcastEpoch() const;
  void setBcastEpoch(EpochType const& epoch);

  NodeType getFromNode() const;
  void setFromNode(NodeType const& node);

  bool getMember() const;
  void setMember(bool const& member);

  bool getWrap() const;
  void setWrap(bool const& wrap);

  #if backend_check_enabled(lblite)
    bool lbLiteInstrument() const;
    void setLBLiteInstrument(bool const& val);
    balance::ElementIDType getElm() const;
    balance::ElementIDType getElmTemp() const;
    void setElm(balance::ElementIDType perm, balance::ElementIDType temp);
    balance::CommCategory getCat() const;
    void setCat(balance::CommCategory cat);
  #endif

  // Explicitly write a parent serializer so derived user messages can contain
  // non-byte serialization
  template <typename SerializerT>
  void serializeParent(SerializerT& s);

  template <typename SerializerT>
  void serializeThis(SerializerT& s);

  friend struct CollectionManager;

private:
  VirtualProxyType bcast_proxy_{};
  VirtualElmProxyType<ColT, IndexType> to_proxy_{};
  HandlerType vt_sub_handler_ = uninitialized_handler;
  EpochType bcast_epoch_ = no_epoch;
  NodeType from_node_ = uninitialized_destination;
  bool member_ = false;
  bool is_wrap_ = false;

  #if backend_check_enabled(lblite)
    /*
     * By default this is off so system messages do not all get
     * instrumented. When the user sends a message through theCollection
     * (sendMsg,broadcastMsg) they are automatically instrumented
     */
    bool lb_lite_instrument_ = false;
    balance::ElementIDType elm_perm_ = 0;
    balance::ElementIDType elm_temp_ = 0;
    balance::CommCategory cat_ = balance::CommCategory::SendRecv;
  #endif
};

}}} /* end namespace vt::vrt::collection */

namespace vt {

template <typename ColT, typename MsgT = ::vt::Message>
using CollectionMessage = vrt::collection::CollectionMessage<ColT, MsgT>;

} /* end namespace vt */

#include "vt/vrt/collection/messages/user.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_USER_H*/
