/*
//@HEADER
// *****************************************************************************
//
//                                 base.impl.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_TYPES_BASE_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_TYPES_BASE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/types/base.h"
#include "vt/vrt/collection/migrate/manager_migrate_attorney.h"
#include "vt/vrt/collection/migrate/migrate_status.h"

#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
typename CollectionBase<ColT, IndexT>::ProxyType
CollectionBase<ColT, IndexT>::getElementProxy(IndexT const& idx) const {
  VirtualElmOnlyProxyType elmProxy;
  VirtualElemProxyBuilder::createElmProxy(elmProxy, idx.uniqueBits());
  ProxyType proxy(this->getProxy(), elmProxy);
  return proxy;
}

template <typename ColT, typename IndexT>
typename CollectionBase<ColT, IndexT>::CollectionProxyType
CollectionBase<ColT, IndexT>::getCollectionProxy() const {
  auto proxy = this->getProxy();
  if (proxy == no_vrt_proxy) {
    proxy = theCollection()->queryProxyContext<IndexT>();
    vtAssertExpr(proxy != no_vrt_proxy);
  }
  typename CollectionBase<ColT, IndexT>::CollectionProxyType col_proxy(proxy);
  return col_proxy;
}

template <typename ColT, typename IndexT>
/*virtual*/ void CollectionBase<ColT, IndexT>::migrate(NodeT const& node) {
  auto const proxy = this->getCollectionProxy();
  auto const index = this->getIndex();
  CollectionElmAttorney<ColT,IndexT>::migrate(proxy(index), node);
}

template <typename ColT, typename IndexT>
template <typename Serializer>
void CollectionBase<ColT, IndexT>::serialize(Serializer& s) {
  Indexable<IndexT>::serialize(s);
}

template <typename ColT, typename IndexT>
/*virtual*/ CollectionBase<ColT, IndexT>::~CollectionBase() {}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_TYPES_BASE_IMPL_H*/
