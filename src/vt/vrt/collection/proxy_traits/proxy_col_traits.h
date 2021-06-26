/*
//@HEADER
// *****************************************************************************
//
//                              proxy_col_traits.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_PROXY_TRAITS_PROXY_COL_TRAITS_H
#define INCLUDED_VT_VRT_COLLECTION_PROXY_TRAITS_PROXY_COL_TRAITS_H

#include "vt/config.h"
#include "vt/vrt/collection/destroy/destroyable.h"
#include "vt/vrt/collection/reducable/reducable.h"
#include "vt/vrt/collection/broadcast/broadcastable.h"
#include "vt/vrt/collection/insert/insert_finished.h"
#include "vt/vrt/collection/rdmaable/rdmaable.h"
#include "vt/vrt/proxy/base_collection_proxy.h"

namespace vt { namespace vrt { namespace collection {

namespace col_proxy {

template <typename ColT, typename IndexT>
using Chain5 = RDMAable<ColT,IndexT,BaseCollectionProxy<ColT,IndexT>>;

template <typename ColT, typename IndexT>
using Chain4 = InsertFinished<ColT,IndexT,Chain5<ColT,IndexT>>;

template <typename ColT, typename IndexT>
using Chain3 = Broadcastable<ColT,IndexT,Chain4<ColT,IndexT>>;

template <typename ColT, typename IndexT>
using Chain2 = Destroyable<ColT,IndexT,Chain3<ColT,IndexT>>;

template <typename ColT, typename IndexT>
using Chain1 = Reducable<ColT,IndexT,Chain2<ColT,IndexT>>;

} /* end namespace proxy */

template <typename ColT, typename IndexT>
struct ProxyCollectionTraits : col_proxy::Chain1<ColT,IndexT> {
  ProxyCollectionTraits() = default;
  ProxyCollectionTraits(ProxyCollectionTraits const&) = default;
  ProxyCollectionTraits(ProxyCollectionTraits&&) = default;
  explicit ProxyCollectionTraits(VirtualProxyType const in_proxy)
    : col_proxy::Chain1<ColT,IndexT>(in_proxy)
  {}
  ProxyCollectionTraits& operator=(ProxyCollectionTraits const&) = default;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_PROXY_TRAITS_PROXY_COL_TRAITS_H*/
