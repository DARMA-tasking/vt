/*
//@HEADER
// *****************************************************************************
//
//                              insertable.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_VRT_COLLECTION_INSERT_INSERTABLE_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_INSERT_INSERTABLE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/insert/insertable.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
ElmInsertable<ColT,IndexT,BaseProxyT>::ElmInsertable(
  typename BaseProxyT::ProxyType const& in_proxy,
  typename BaseProxyT::ElementProxyType const& in_elm
) : BaseProxyT(in_proxy, in_elm)
{ }

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename SerializerT>
void ElmInsertable<ColT,IndexT,BaseProxyT>::serialize(SerializerT& s) {
  BaseProxyT::serialize(s);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
void ElmInsertable<ColT,IndexT,BaseProxyT>::insert(ModifierToken& token) const {
  auto const col_proxy = this->getCollectionProxy();
  auto const elm_proxy = this->getElementProxy();
  auto const idx = elm_proxy.getIndex();
  theCollection()->insert<ColT, InsertNullMsg>(
    col_proxy, idx, uninitialized_destination, token
  );
}

template <typename ColT, typename IndexT, typename BaseProxyT>
void ElmInsertable<ColT,IndexT,BaseProxyT>::insertAt(
  ModifierToken& token, NodeType node
) const {
  auto const col_proxy = this->getCollectionProxy();
  auto const elm_proxy = this->getElementProxy();
  auto const idx = elm_proxy.getIndex();
  theCollection()->insert<ColT, InsertNullMsg>(col_proxy, idx, node, token);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT>
void ElmInsertable<ColT,IndexT,BaseProxyT>::insertAtMsg(
  ModifierToken& token, NodeType node, messaging::MsgPtrThief<MsgT> msg
) const {
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  auto const col_proxy = this->getCollectionProxy();
  auto const elm_proxy = this->getElementProxy();
  auto const idx = elm_proxy.getIndex();
  theCollection()->insert<ColT, MsgT>(col_proxy, idx, node, token, msgptr);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename MsgT>
void ElmInsertable<ColT,IndexT,BaseProxyT>::insertMsg(
  ModifierToken& token, messaging::MsgPtrThief<MsgT> msg
) const {
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  auto const col_proxy = this->getCollectionProxy();
  auto const elm_proxy = this->getElementProxy();
  auto const idx = elm_proxy.getIndex();
  theCollection()->insert<ColT, MsgT>(
    col_proxy, idx, uninitialized_destination, token, msgptr
  );
}
template <typename ColT, typename IndexT, typename BaseProxyT>
void ElmInsertable<ColT,IndexT,BaseProxyT>::destroy(ModifierToken& token) const {
  auto const col_proxy = this->getCollectionProxy();
  auto const elm_proxy = this->getElementProxy();
  auto const idx = elm_proxy.getIndex();
  theCollection()->destroyElm<ColT>(col_proxy, idx, token);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_INSERT_INSERTABLE_IMPL_H*/
