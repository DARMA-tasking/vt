/*
//@HEADER
// *****************************************************************************
//
//                              typeless_holder.cc
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

#include "vt/vrt/collection/holders/typeless_holder.h"

namespace vt { namespace vrt { namespace collection {

void TypelessHolder::destroyAllLive() {
  for (auto&& elm : live_) {
    elm.second->destroy();
  }
  live_.clear();
}

void TypelessHolder::destroyCollection(VirtualProxyType const proxy) {
  auto iter = live_.find(proxy);
  if (iter != live_.end()) {
    live_.erase(iter);
  }
}

void TypelessHolder::insertCollectionInfo(
  VirtualProxyType const proxy, std::shared_ptr<BaseHolder> ptr
) {
  live_[proxy] = ptr;
}

void TypelessHolder::insertMap(
  VirtualProxyType const proxy, HandlerType const map_han
) {
  map_[proxy] = map_han;
}

HandlerType TypelessHolder::getMap(VirtualProxyType const proxy) {
  auto map_iter = map_.find(proxy);
  vtAssert(map_iter != map_.end(), "Map must exist");
  return map_iter->second;
}

}}} /* end namespace vt::vrt::collection */
