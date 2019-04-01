/*
//@HEADER
// ************************************************************************
//
//                          user.impl.h
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

#if !defined INCLUDED_VRT_COLLECTION_MESSAGES_USER_IMPL_H
#define INCLUDED_VRT_COLLECTION_MESSAGES_USER_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/messages/user.h"

#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setVrtHandler(
  HandlerType const& in_handler
) {
  vt_sub_handler_ = in_handler;
}

template <typename ColT, typename BaseMsgT>
HandlerType CollectionMessage<ColT, BaseMsgT>::getVrtHandler() const {
  vtAssert(
    vt_sub_handler_ != uninitialized_handler, "Must have a valid handler"
  );
  return vt_sub_handler_;
}

template <typename ColT, typename BaseMsgT>
VirtualElmProxyType<ColT, typename ColT::IndexType>
CollectionMessage<ColT, BaseMsgT>::getProxy() const {
  return to_proxy_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setProxy(
  VirtualElmProxyType<ColT, typename ColT::IndexType> const& in_proxy
) {
  to_proxy_ = in_proxy;
}

template <typename ColT, typename BaseMsgT>
VirtualProxyType
CollectionMessage<ColT, BaseMsgT>::getBcastProxy() const {
  return bcast_proxy_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setBcastProxy(
  VirtualProxyType const& in_proxy
) {
  bcast_proxy_ = in_proxy;
}

template <typename ColT, typename BaseMsgT>
EpochType CollectionMessage<ColT, BaseMsgT>::getBcastEpoch() const {
  return bcast_epoch_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setBcastEpoch(
  EpochType const& epoch
) {
  bcast_epoch_ = epoch;
}

template <typename ColT, typename BaseMsgT>
NodeType CollectionMessage<ColT, BaseMsgT>::getFromNode() const {
  return from_node_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setFromNode(NodeType const& node) {
  from_node_ = node;
}

template <typename ColT, typename BaseMsgT>
template <typename SerializerT>
void CollectionMessage<ColT, BaseMsgT>::serializeParent(SerializerT& s) {
  RoutedMessageType<BaseMsgT, ColT>::serializeParent(s);
  RoutedMessageType<BaseMsgT, ColT>::serializeThis(s);
}

template <typename ColT, typename BaseMsgT>
template <typename SerializerT>
void CollectionMessage<ColT, BaseMsgT>::serializeThis(SerializerT& s) {
  s | vt_sub_handler_;
  s | to_proxy_;
  s | bcast_proxy_;
  s | bcast_epoch_;
  s | member_;
  s | is_wrap_;

  backend_enable_if(
    lblite,
    s | lb_lite_instrument_;
  );
}

template <typename ColT, typename BaseMsgT>
bool CollectionMessage<ColT, BaseMsgT>::getMember() const {
  return member_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setMember(bool const& member) {
  member_ = member;
}

template <typename ColT, typename BaseMsgT>
bool CollectionMessage<ColT, BaseMsgT>::getWrap() const {
  return is_wrap_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setWrap(bool const& wrap) {
  is_wrap_ = wrap;
}

template <typename ColT, typename BaseMsgT>
typename ColT::IndexType CollectionMessage<ColT, BaseMsgT>::getRange() const {
  return range_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setRange(
  typename ColT::IndexType const& in_range
) {
  range_ = in_range;
}

template <typename ColT, typename BaseMsgT>
bool CollectionMessage<ColT, BaseMsgT>::isView() const {
  return is_view_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setViewFlag(bool const& in_view) {
  is_view_ = in_view;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setViewProxy(
  vt::VirtualProxyType const& proxy
) {
  view_proxy_ = proxy;
}

template <typename ColT, typename BaseMsgT>
VirtualProxyType const& CollectionMessage<ColT, BaseMsgT>::getViewProxy() {
  return view_proxy_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setViewHandler(
  vt::HandlerType const& in_handler
) {
  view_handler_ = in_handler;
}

template <typename ColT, typename BaseMsgT>
HandlerType CollectionMessage<ColT, BaseMsgT>::getViewHandler() const {
  return view_handler_;
}

#if backend_check_enabled(lblite)
template <typename ColT, typename BaseMsgT>
bool CollectionMessage<ColT, BaseMsgT>::lbLiteInstrument() const {
  return lb_lite_instrument_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setLBLiteInstrument(bool const& val) {
  lb_lite_instrument_ = val;
}
#endif

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_USER_IMPL_H*/
