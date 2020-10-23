/*
//@HEADER
// *****************************************************************************
//
//                               entire_holder.h
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
  static void remove(VirtualProxyType const& proxy);
  static ProxyContainerType proxy_container_;
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/holders/entire_holder.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_ENTIRE_HOLDER_H*/
