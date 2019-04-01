/*
//@HEADER
// ************************************************************************
//
//                          collection_proxy.impl.h
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

#if !defined INCLUDED_VRT_PROXY_COLLECTION_PROXY_IMPL_H
#define INCLUDED_VRT_PROXY_COLLECTION_PROXY_IMPL_H

#include "vt/config.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/vrt/proxy/base_elm_proxy.h"
#include "vt/vrt/collection/proxy_traits/proxy_col_traits.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
CollectionProxy<ColT, IndexT>::CollectionProxy(
  VirtualProxyType const in_proxy
) : ProxyCollectionTraits<ColT, IndexT>(in_proxy)
{ }

template <typename ColT, typename IndexT>
CollectionProxy<ColT, IndexT>::CollectionProxy(
  VirtualProxyType const in_proxy, IndexT const in_range
) : ProxyCollectionTraits<ColT, IndexT>(in_proxy, in_range)
{ }

template <typename ColT, typename IndexT>
template <typename... IndexArgsT>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::index_build(IndexArgsT&&... args) const {
  using BaseIndexType = typename IndexT::DenseIndexType;
  return index(IndexT(static_cast<BaseIndexType>(args)...));
}

template <typename ColT, typename IndexT>
template <typename... IndexArgsT>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::operator[](IndexArgsT&&... args) const {
  return index_build(std::forward<IndexArgsT>(args)...);
}

template <typename ColT, typename IndexT>
template <typename... IndexArgsT>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::operator()(IndexArgsT&&... args) const {
  return index_build(std::forward<IndexArgsT>(args)...);
}

template <typename ColT, typename IndexT>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::index(IndexT const& idx) const {
  // todo: update here for relative indexing
  return ElmProxyType{this->proxy_,BaseElmProxy<ColT, IndexT>{idx}};
}

template <typename ColT, typename IndexT>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::operator[](IndexT const& idx) const {
  return index(idx);
}

template <typename ColT, typename IndexT>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::operator()(IndexT const& idx) const {
  return index(idx);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_PROXY_COLLECTION_PROXY_IMPL_H*/
