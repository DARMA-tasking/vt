/*
//@HEADER
// ************************************************************************
//
//                          entire_holder.h
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

#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_H

#include "vt/config.h"
#include "vt/vrt/collection/holders/col_holder.h"
#include "vt/epoch/epoch.h"

#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace vt { namespace vrt { namespace collection {

template <typename=void>
struct UniversalIndexHolder {
  static void destroyAllLive();
  static void destroyCollection(VirtualProxyType const proxy);
  static bool readyNextPhase();
  static void makeCollectionReady(VirtualProxyType const proxy);
  static void resetPhase();
  static std::size_t getNumCollections();
  static std::size_t getNumReadyCollections();
  static void insertMap(
    VirtualProxyType const proxy, HandlerType const& han,
    EpochType const& insert_epoch = no_epoch
  );
  static HandlerType getMap(VirtualProxyType const proxy);
public:
  static void insertSetEpoch(
    VirtualProxyType const proxy, EpochType const& insert_epoch
  );
  static EpochType insertGetEpoch(VirtualProxyType const proxy);
  static std::unordered_map<VirtualProxyType,EpochType> insert_epoch_;
  static std::unordered_set<VirtualProxyType> ready_collections_;
  static std::unordered_map<
    VirtualProxyType,std::shared_ptr<BaseHolder>
  > live_collections_;
  static std::unordered_map<VirtualProxyType,HandlerType> live_collections_map_;
private:
  static std::size_t num_collections_phase_;
};

template <typename ColT, typename IndexT>
struct EntireHolder {
  using InnerHolder = CollectionHolder<ColT, IndexT>;
  using InnerHolderPtr = std::shared_ptr<InnerHolder>;
  using ProxyContainerType = std::unordered_map<
    VirtualProxyType, InnerHolderPtr
  >;
  static void insert(VirtualProxyType const& proxy, InnerHolderPtr ptr);
  static ProxyContainerType proxy_container_;
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/holders/entire_holder.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_H*/
