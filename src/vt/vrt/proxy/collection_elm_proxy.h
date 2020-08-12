/*
//@HEADER
// *****************************************************************************
//
//                            collection_elm_proxy.h
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

#if !defined INCLUDED_VRT_PROXY_COLLECTION_ELM_PROXY_H
#define INCLUDED_VRT_PROXY_COLLECTION_ELM_PROXY_H

#include "vt/config.h"
#include "vt/vrt/collection/proxy_traits/proxy_elm_traits.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/collection/send/sendable.h"
#include "vt/vrt/collection/insert/insertable.h"
#include "vt/vrt/proxy/base_elm_proxy.h"

#include <iosfwd>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct VrtElmProxy : ProxyCollectionElmTraits<ColT, IndexT> {
  using CollectionProxyType =
    typename ProxyCollectionElmTraits<ColT, IndexT>::ProxyType;
  using ElementProxyType =
    typename ProxyCollectionElmTraits<ColT, IndexT>::ElementProxyType;

  VrtElmProxy(
    VirtualProxyType const& in_col_proxy,
    BaseElmProxy<IndexT> const& in_elm_proxy
  ) : ProxyCollectionElmTraits<ColT, IndexT>(in_col_proxy, in_elm_proxy)
  { }

  VrtElmProxy(
    VirtualProxyType const& in_col_proxy, IndexT const& in_index
  ) : VrtElmProxy(in_col_proxy, ElementProxyType{in_index})
  { }

  VrtElmProxy() = default;
  VrtElmProxy(VrtElmProxy const&) = default;
  VrtElmProxy(VrtElmProxy&&) = default;
  VrtElmProxy& operator=(VrtElmProxy const&) = default;

  bool operator==(VrtElmProxy const& other) const {
    return other.col_proxy_ == this->col_proxy_ &&
           other.elm_proxy_ == this->elm_proxy_;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    ProxyCollectionElmTraits<ColT, IndexT>::serialize(s);
  }

  template <typename ColU, typename IndexU>
  friend std::ostream& operator<<(
    std::ostream& os, VrtElmProxy<ColU,IndexU> const& vrt
  );

  friend struct CollectionManager;
};

template <typename ColT, typename IndexT>
std::ostream& operator<<(
  std::ostream& os, VrtElmProxy<ColT,IndexT> const& vrt
) {
  os << "("
     << "vrt=" << vrt.col_proxy_ << ","
     << "idx=" << vrt.elm_proxy_.getIndex()
     << ")";
  return os;
}

}}} /* end namespace vt::vrt::collection */

template <typename ColT, typename IndexT>
using ElmProxyType = ::vt::vrt::collection::VrtElmProxy<ColT, IndexT>;

namespace std {
template <typename ColT, typename IndexT>
struct hash<ElmProxyType<ColT, IndexT>> {
  size_t operator()(ElmProxyType<ColT, IndexT> const& in) const {
    return
      std::hash<typename ElmProxyType<ColT, IndexT>::CollectionProxyType>()(
        in.getCollectionProxy()
      ) +
      std::hash<typename ElmProxyType<ColT, IndexT>::ElementProxyType>()(
        in.getElementProxy()
      );
  }
};
}

#endif /*INCLUDED_VRT_PROXY_COLLECTION_ELM_PROXY_H*/
