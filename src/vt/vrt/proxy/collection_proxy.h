/*
//@HEADER
// *****************************************************************************
//
//                              collection_proxy.h
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

#if !defined INCLUDED_VRT_PROXY_COLLECTION_PROXY_H
#define INCLUDED_VRT_PROXY_COLLECTION_PROXY_H

#include "vt/config.h"
#include "vt/vrt/collection/proxy_traits/proxy_col_traits.h"
#include "vt/vrt/proxy/base_elm_proxy.h"
#include "vt/vrt/proxy/collection_elm_proxy.h"
#include "vt/vrt/proxy/base_collection_proxy.h"
#include "vt/vrt/collection/balance/elm_stats.h"

namespace vt { namespace vrt { namespace collection {

/*
 * `CollectionProxy' (variant with IndexT baked into class, allowing
 * constructors to be forwarded for building indices in line without the type.
 */

template <typename ColT, typename IndexT = typename ColT::IndexType>
struct CollectionProxy : ProxyCollectionTraits<ColT, IndexT> {
  using ElmProxyType = VrtElmProxy<ColT, IndexT>;

  CollectionProxy() = default;
  CollectionProxy(CollectionProxy const&) = default;
  CollectionProxy(VirtualProxyType const in_proxy);

  CollectionProxy& operator=(CollectionProxy const&) = default;

  template <typename... IndexArgsT>
  ElmProxyType indexArgs(IndexArgsT&&... idx) const;
  ElmProxyType index(IndexT const& idx) const;

  template <
    typename Tp,
    typename   = typename std::enable_if<
      std::is_convertible<
        typename IndexT::BuildIndexType, typename std::decay<Tp>::type
      >::value, Tp
    >::type
  >
  ElmProxyType operator[](Tp&& tp) const;

  template <
    typename Tp, typename... Tn,
    typename   = typename std::enable_if<
      std::is_convertible<
        typename IndexT::BuildIndexType, typename std::decay<Tp>::type
      >::value, Tp
    >::type
  >
  ElmProxyType operator()(Tp&& tp, Tn&&... tn) const;

  template <
    typename IndexU,
    typename   = typename std::enable_if<
      std::is_same<IndexT, typename std::decay<IndexU>::type>::value, IndexU
    >::type
  >
  ElmProxyType operator[](IndexU const& idx) const;

  template <
    typename IndexU,
    typename   = typename std::enable_if<
      std::is_same<IndexT, typename std::decay<IndexU>::type>::value, IndexU
    >::type
  >
  ElmProxyType operator()(IndexU const& idx) const;

  void setFocusedSubPhase(balance::ElementStats::SubphaseType subphase);
};

}}} /* end namespace vt::vrt::collection */

namespace vt {

template <typename ColT, typename IndexT>
using CollectionIndexProxy = vrt::collection::CollectionProxy<ColT,IndexT>;

template <typename ColT, typename IndexT>
using CollectionProxy = vrt::collection::CollectionProxy<ColT,IndexT>;

} /* end namespace vt */

#include "vt/vrt/proxy/collection_proxy.impl.h"

#endif /*INCLUDED_VRT_PROXY_COLLECTION_PROXY_H*/
