/*
//@HEADER
// *****************************************************************************
//
//                              proxy_elm_traits.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_PROXY_TRAITS_PROXY_ELM_TRAITS_H
#define INCLUDED_VT_VRT_COLLECTION_PROXY_TRAITS_PROXY_ELM_TRAITS_H

#include "vt/config.h"
#include "vt/vrt/proxy/base_collection_elm_proxy.h"
#include "vt/vrt/proxy/base_elm_proxy.h"
#include "vt/vrt/collection/send/sendable.h"
#include "vt/vrt/collection/invoke/invokable.h"
#include "vt/vrt/collection/gettable/gettable.h"
#include "vt/vrt/collection/insert/insertable.h"

namespace vt { namespace vrt { namespace collection {

namespace elm_proxy {

template <typename ColT, typename IndexT>
using Chain4 = Invokable<ColT,IndexT,BaseCollectionElmProxy<IndexT>>;

template <typename ColT, typename IndexT>
using Chain3 = Gettable<ColT,IndexT,Chain4<ColT,IndexT>>;

template <typename ColT, typename IndexT>
using Chain2 = ElmInsertable<ColT,IndexT,Chain3<ColT,IndexT>>;

template <typename ColT, typename IndexT>
using Chain1 = Sendable<ColT,IndexT,Chain2<ColT,IndexT>>;

} /* end namespace proxy */

template <typename ColT, typename IndexT>
struct ProxyCollectionElmTraits : elm_proxy::Chain1<ColT,IndexT> {
  ProxyCollectionElmTraits() = default;
  ProxyCollectionElmTraits(ProxyCollectionElmTraits const&) = default;
  ProxyCollectionElmTraits(ProxyCollectionElmTraits&&) = default;
  ProxyCollectionElmTraits(
    typename elm_proxy::Chain1<ColT,IndexT>::ProxyType const& in_proxy,
    typename elm_proxy::Chain1<ColT,IndexT>::ElementProxyType const& in_elm
  ) : elm_proxy::Chain1<ColT,IndexT>(in_proxy,in_elm)
  {}
  ProxyCollectionElmTraits& operator=(ProxyCollectionElmTraits const&) = default;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_PROXY_TRAITS_PROXY_ELM_TRAITS_H*/
