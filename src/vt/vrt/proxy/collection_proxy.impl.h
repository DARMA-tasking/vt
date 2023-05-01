/*
//@HEADER
// *****************************************************************************
//
//                           collection_proxy.impl.h
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

#if !defined INCLUDED_VT_VRT_PROXY_COLLECTION_PROXY_IMPL_H
#define INCLUDED_VT_VRT_PROXY_COLLECTION_PROXY_IMPL_H

#include "vt/config.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/vrt/proxy/base_elm_proxy.h"
#include "vt/vrt/collection/proxy_traits/proxy_col_traits.h"
#include "vt/vrt/collection/balance/col_lb_data.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
CollectionProxy<ColT, IndexT>::CollectionProxy(
  VirtualProxyType const in_proxy
) : ProxyCollectionTraits<ColT, IndexT>(in_proxy)
{ }

template <typename ColT, typename IndexT>
template <typename... IndexArgsT>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::indexArgs(IndexArgsT&&... args) const {
  using BaseIndexType = typename IndexT::DenseIndexType;
  return index(IndexT(static_cast<BaseIndexType>(args)...));
}

template <typename ColT, typename IndexT>
template <typename Tp, typename _unused>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::operator[](Tp&& tp) const {
  return indexArgs(std::forward<Tp>(tp));
}

template <typename ColT, typename IndexT>
template <typename Tp, typename... Tn, typename _unused>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::operator()(Tp&& tp, Tn&&... tn) const {
  return indexArgs(std::forward<Tp>(tp),std::forward<Tn>(tn)...);
}

template <typename ColT, typename IndexT>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::index(IndexT const& idx) const {
  return ElmProxyType{this->proxy_,BaseElmProxy<IndexT>{idx}};
}

template <typename ColT, typename IndexT>
template <typename IndexU, typename _unused>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::operator[](IndexU const& idx) const {
  return index(idx);
}

template <typename ColT, typename IndexT>
template <typename IndexU, typename _unused>
typename CollectionProxy<ColT, IndexT>::ElmProxyType
CollectionProxy<ColT, IndexT>::operator()(IndexU const& idx) const {
  return index(idx);
}

template <typename ColT, typename IndexT>
void
CollectionProxy<ColT, IndexT>::setFocusedSubPhase(SubphaseType subphase) {
  balance::CollectionLBData::setFocusedSubPhase(this->getProxy(), subphase);
}

template <typename ColT, typename IndexT>
template <typename T>
void CollectionProxy<ColT, IndexT>::serialize(
    typename DefaultSerializer<T>::type& s
){
  ProxyCollectionTraits<ColT, IndexT>::serialize(s);
}

template <typename ColT, typename IndexT>
template <typename T>
void CollectionProxy<ColT, IndexT>::serialize(
    typename CheckpointSerializer<T>::type& s
){
  
  //Save the proxy info we had before, to detect when 
  //we need to create the proxy while unpacking.
  VirtualProxyType oldProxy = this->getProxy();
  vtAssert(oldProxy != no_vrt_proxy || !s.isPacking(),
           "Proxy must be instantiated to checkpoint");

  //Serialize parent class info, which handles proxy bits.
  ProxyCollectionTraits<ColT, IndexT>::serialize(s);
  VirtualProxyType proxy = this->getProxy();

  std::string label;
  if(!s.isUnpacking()) label = vt::theCollection()->getLabel(proxy);
  s | label;

  if(s.isUnpacking() && oldProxy == proxy) { 
    vtAssert(label == vt::theCollection()->getLabel(proxy),
             "Checkpointed proxy and deserialize target labels do not match!");
  }

  
  std::set<IndexT> localElmSet;
  if(!s.isUnpacking()) localElmSet = vt::theCollection()->getLocalIndices(*this);
  s | localElmSet;

  IndexT bounds;
  if(!s.isUnpacking()) bounds = vt::theCollection()->getRange<ColT>(proxy);
  s | bounds;

  bool isDynamic;
  if(!s.isUnpacking()) 
    isDynamic = vt::theCollection()->getDynamicMembership<ColT>(proxy);
  s | isDynamic;
  
  //TODO: magistrate's virtualized serialization support may enable checkpointing
  //mapper objects. Pre-registered functions could be doable as well.

  //TODO: chkpt location manager so we don't have to message back and forth so much?
  //auto lm = theLocMan()->getCollectionLM<IndexType>(proxy.getProxy());

 

  //If unpacking, we may need to make the collection to unpack into.
  if(s.isUnpacking() && oldProxy != proxy){
    vtAssert(oldProxy == no_vrt_proxy,
             "Checkpointed proxy and deserialize target do not match!");
    
    //The checkpointed proxy doesn't exist, we need to create it 
    //First get all of the element unique_ptrs to hand off to the constructor
    std::vector<std::tuple<IndexT, ElmProxyType>> localElms;
    for(auto& idx : localElmSet){
      //Tell elements not to try verifying placement/migrating.
      localElms.emplace_back(std::make_tuple(idx, 
                          std::move(ElmProxyType::deserializeToElm(s))));
    }
    
    bool is_collective = !VirtualProxyBuilder::isRemote(proxy);
    vtAssert(is_collective, "VT requires collective collections to enable \
             list inserting and custom constructing, cannot recover a rooted \
             collection.");

    vt::makeCollection<ColT>(label)
      .bounds(bounds)
      .dynamicMembership(isDynamic)
      .proxyBits(proxy)
      .listInsertHere(std::move(localElms))
      .deferWithEpoch([=](VirtualProxyType assigned_proxy){
            vtAssert(assigned_proxy == proxy, "Proxy must be assigned as expected (for now)");
      });
  } else {
    //We're serializing/deserializing in-place
    //Serialize each element itself, the elements will handle
    //requesting migrations as needed.
    for(auto& elm_idx : localElmSet){
      s | (*this)(elm_idx);
    }
  }
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_PROXY_COLLECTION_PROXY_IMPL_H*/
