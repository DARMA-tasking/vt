/*
//@HEADER
// *****************************************************************************
//
//                                 user.impl.h
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

#if !defined INCLUDED_VRT_COLLECTION_MESSAGES_USER_IMPL_H
#define INCLUDED_VRT_COLLECTION_MESSAGES_USER_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/messages/user.h"

#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setVrtHandler(
  HandlerType const in_handler
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
VirtualProxyType CollectionMessage<ColT, BaseMsgT>::getBcastProxy() const {
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
void CollectionMessage<ColT, BaseMsgT>::setBcastEpoch(EpochType const& epoch) {
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
void CollectionMessage<ColT, BaseMsgT>::serialize(SerializerT& s) {
  MessageParentType::serialize(s);
  s | vt_sub_handler_;
  s | to_proxy_;
  s | bcast_proxy_;
  s | bcast_epoch_;
  s | is_wrap_;
  s | from_node_;

  #if vt_check_enabled(lblite)
    s | lb_lite_instrument_;
    s | elm_;
    s | cat_;
  #endif

  #if vt_check_enabled(trace_enabled)
    s | trace_event_;
  #endif
}

template <typename ColT, typename BaseMsgT>
bool CollectionMessage<ColT,BaseMsgT>::getWrap() const {
  return is_wrap_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT,BaseMsgT>::setWrap(bool const& wrap) {
  is_wrap_ = wrap;
}

#if vt_check_enabled(lblite)
template <typename ColT, typename BaseMsgT>
bool CollectionMessage<ColT, BaseMsgT>::lbLiteInstrument() const {
  return lb_lite_instrument_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setLBLiteInstrument(bool const& val) {
  lb_lite_instrument_ = val;
}

template <typename ColT, typename BaseMsgT>
balance::ElementIDStruct CollectionMessage<ColT, BaseMsgT>::getElm() const {
  return elm_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setElm(
  balance::ElementIDStruct elm
) {
  elm_ = elm;
}

template <typename ColT, typename BaseMsgT>
balance::CommCategory CollectionMessage<ColT, BaseMsgT>::getCat() const {
  return cat_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setCat(balance::CommCategory cat) {
  cat_ = cat;
}


#endif

#if vt_check_enabled(trace_enabled)

template <typename ColT, typename BaseMsgT>
trace::TraceEventIDType
CollectionMessage<ColT, BaseMsgT>::getFromTraceEvent() const {
  return trace_event_;
}

template <typename ColT, typename BaseMsgT>
void CollectionMessage<ColT, BaseMsgT>::setFromTraceEvent(
  trace::TraceEventIDType in_event
) {
  trace_event_ = in_event;
}

#endif

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_USER_IMPL_H*/
