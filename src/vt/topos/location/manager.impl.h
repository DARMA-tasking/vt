/*
//@HEADER
// *****************************************************************************
//
//                                manager.impl.h
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

#if !defined INCLUDED_VT_TOPOS_LOCATION_MANAGER_IMPL_H
#define INCLUDED_VT_TOPOS_LOCATION_MANAGER_IMPL_H

#include "vt/config.h"
#include "vt/topos/location/manager.h"
#include "vt/topos/location/location_common.h"
#include "vt/topos/location/location.h"

#include <memory>
#include <cassert>
#include <unordered_map>
#include <functional>

namespace vt { namespace location {

template <typename IndexT>
typename LocationManager::IndexedElmType<IndexT>*
LocationManager::getCollectionLM(VirtualProxyType proxy) {
  if (auto it = collection_lms.find(proxy); it != collection_lms.end()) {
    return objgroup::proxy::Proxy<IndexedElmType<IndexT>>(it->second).get();
  } else {
    vtAbort("Could not find location manager for proxy");
    return nullptr;
  }
}

template <typename IndexT>
objgroup::proxy::Proxy<LocationManager::IndexedElmType<IndexT>>
LocationManager::makeCollectionLM(VirtualProxyType proxy) {
  using LocType = IndexedElmType<IndexT>;
  auto lm_proxy = theObjGroup()->makeCollective<LocType>("LocationManager");
  lm_proxy.get()->setProxy(lm_proxy);
  collection_lms[proxy] = lm_proxy.getProxy();
  return lm_proxy;
}

template <typename IndexT>
void LocationManager::destroyCollectionLM(VirtualProxyType proxy) {
  if (auto elm = collection_lms.extract(proxy); elm) {
    objgroup::proxy::Proxy<IndexedElmType<IndexT>> lm_proxy(elm.mapped());
    lm_proxy.destroyCollective();
  } else {
    vtAbort("Could not find location manager for proxy");
  }
}

}} /* end namespace vt::location */

#endif /*INCLUDED_VT_TOPOS_LOCATION_MANAGER_IMPL_H*/
