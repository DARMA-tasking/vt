/*
//@HEADER
// *****************************************************************************
//
//                               handlable.impl.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_RMA_HANDLABLE_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_RMA_HANDLABLE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/rma/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
Handlable<ColT,IndexT,BaseProxyT>::Handlable(
  typename BaseProxyT::ProxyType const& in_proxy,
  typename BaseProxyT::ElementProxyType const& in_elm
) : BaseProxyT(in_proxy, in_elm)
{ }

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename SerializerT>
void Handlable<ColT,IndexT,BaseProxyT>::serialize(SerializerT& s) {
  BaseProxyT::serialize(s);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename HanT, ActiveHandleTypedFnType<HanT,ColT> f>
int Handlable<ColT,IndexT,BaseProxyT>::atomicPush(int elms, HanT data) const {
  auto col_proxy = this->getCollectionProxy();
  auto elm_proxy = this->getElementProxy();
  auto tproxy = CollectionProxy<ColT, IndexT>(col_proxy);

  auto this_idx = theCollection()->queryIndexContext<IndexT>();

  auto ptr = tproxy[*this_idx].tryGetLocalPtr();
  vtAssert(ptr != nullptr, "Must be valid pointer");
  auto handle = auto_registry::makeAutoHandlerCollectionHan<ColT,HanT,f>();

  auto idx = elm_proxy.getIndex();
  auto tup = ptr->global_local_[handle][idx];
  auto rank = std::get<0>(tup);
  auto slot = std::get<1>(tup);

  return rma::Manager::push<ColT, HanT>(handle, rank, slot, idx, elms, data);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename HanT, ActiveHandleTypedFnType<HanT,ColT> f>
int Handlable<ColT,IndexT,BaseProxyT>::atomicPop(HanT data) const {
  auto col_proxy = this->getCollectionProxy();
  auto elm_proxy = this->getElementProxy();
  auto tproxy = CollectionProxy<ColT, IndexT>(col_proxy);

  auto this_idx = theCollection()->queryIndexContext<IndexT>();

  auto ptr = tproxy[*this_idx].tryGetLocalPtr();
  vtAssert(ptr != nullptr, "Must be valid pointer");
  auto handle = auto_registry::makeAutoHandlerCollectionHan<ColT,HanT,f>();

  auto idx = elm_proxy.getIndex();
  auto tup = ptr->global_local_[handle][idx];
  auto rank = std::get<0>(tup);
  auto slot = std::get<1>(tup);

  return rma::Manager::pop<ColT, HanT>(handle, rank, slot, idx, data);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename HanT, ActiveHandleTypedFnType<HanT,ColT> f>
int Handlable<ColT,IndexT,BaseProxyT>::atomicGetAccum(int val) const {
  auto col_proxy = this->getCollectionProxy();
  auto elm_proxy = this->getElementProxy();
  auto tproxy = CollectionProxy<ColT, IndexT>(col_proxy);

  auto this_idx = theCollection()->queryIndexContext<IndexT>();

  auto ptr = tproxy[*this_idx].tryGetLocalPtr();
  vtAssert(ptr != nullptr, "Must be valid pointer");
  auto handle = auto_registry::makeAutoHandlerCollectionHan<ColT,HanT,f>();

  auto tup = ptr->global_local_[handle][elm_proxy.getIndex()];
  auto rank = std::get<0>(tup);
  auto slot = std::get<1>(tup);

  return rma::Manager::atomicGetAccum<ColT>(handle, rank, slot, val);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename HanT, ActiveHandleTypedFnType<HanT,ColT> f>
void Handlable<ColT,IndexT,BaseProxyT>::connect(
  typename ColT::IndexType from
) const {
  auto col_proxy = this->getCollectionProxy();
  auto elm_proxy = this->getElementProxy();
  auto proxy = VrtElmProxy<ColT, IndexT>(col_proxy,elm_proxy);
  auto rank = theContext()->getNode();
  auto handle = auto_registry::makeAutoHandlerCollectionHan<ColT,HanT,f>();
  proxy.template send<rma::ConnectMsg<ColT>, rma::Manager::connect<ColT>>(
    static_cast<HandleType>(handle), from, rank
  );
}

template <typename ColT, typename IndexT, typename BaseProxyT>
template <typename HanT, ActiveHandleTypedFnType<HanT,ColT> f>
void Handlable<ColT,IndexT,BaseProxyT>::registerHandle() const {
  auto elm_proxy = this->getElementProxy();

  auto handle = auto_registry::makeAutoHandlerCollectionHan<ColT,HanT,f>();
  rma::Manager::addLocalHandle<ColT>(
    static_cast<HandleType>(handle), elm_proxy.getIndex()
  );
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_RMA_HANDLABLE_IMPL_H*/
