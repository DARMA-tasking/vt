/*
//@HEADER
// *****************************************************************************
//
//                              typeless_holder.cc
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

#include "vt/vrt/collection/holders/typeless_holder.h"
#include "vt/scheduler/scheduler.h"
#include "vt/collective/reduce/allreduce/state_holder.h"
#include "vt/collective/reduce/allreduce/allreduce_holder.h"

namespace vt { namespace vrt { namespace collection {

void TypelessHolder::destroyAllLive() {
  for (auto&& elm : live_) {
    elm.second->destroy();
  }
  live_.clear();
  group_constructors_.clear();
}

void TypelessHolder::destroyCollection(VirtualProxyType const proxy) {
  {
    auto iter = live_.find(proxy);
    if (iter != live_.end()) {
      live_.erase(iter);
    }
  }
  {
    auto iter = group_constructors_.find(proxy);
    if (iter != group_constructors_.end()) {
      group_constructors_.erase(iter);
    }
  }
   {
    auto iter = labels_.find(proxy);
    if (iter != labels_.end()) {
      labels_.erase(iter);
    }
  }

  vt::collective::reduce::allreduce::StateHolder::clearAll(
    vt::collective::reduce::detail::StrongVrtProxy{proxy});

  vt::collective::reduce::allreduce::AllreduceHolder::remove(
    vt::collective::reduce::detail::StrongVrtProxy{proxy}
  );
}

void TypelessHolder::invokeAllGroupConstructors() {
  for (auto&& proxy : live_) {
    runInEpochCollective([&]{
      auto iter = group_constructors_.find(proxy.first);
      vtAssert(iter != group_constructors_.end(), "Must have group constructor");
      iter->second();
    });
  }
}

void TypelessHolder::insertCollectionInfo(
  VirtualProxyType const proxy, std::shared_ptr<BaseHolder> ptr,
  std::function<void()> group_constructor, std::string const& label
) {
  live_[proxy] = ptr;
  group_constructors_[proxy] = group_constructor;
  labels_[proxy] = label;
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

std::string TypelessHolder::getLabel(VirtualProxyType const proxy) const {
  auto const iter = labels_.find(proxy);
  vtAssert(iter != labels_.cend(), "Label does not exist");
  return iter->second;
}

}}} /* end namespace vt::vrt::collection */
