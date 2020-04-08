/*
//@HEADER
// *****************************************************************************
//
//                                lookup.impl.h
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

#if !defined INCLUDED_VT_TOPOS_LOCATION_LOOKUP_LOOKUP_IMPL_H
#define INCLUDED_VT_TOPOS_LOCATION_LOOKUP_LOOKUP_IMPL_H

#include "vt/config.h"

namespace vt { namespace location {

template <typename KeyT, typename ValueT>
bool LocLookup<KeyT, ValueT>::exists(KeyT const& key) const {
  return directory_.exists(key) or cache_.exists(key);
}

template <typename KeyT, typename ValueT>
ValueT const& LocLookup<KeyT, ValueT>::get(KeyT const& key) {
  auto dir_iter = directory_.getIter(key);
  if (dir_iter != directory_.getIterEnd()) {
    return dir_iter->second;
  }
  return cache_.get(key);
}

template <typename KeyT, typename ValueT>
LocationSizeType LocLookup<KeyT, ValueT>::getCacheSize() const {
  return cache_.getSize();
}

template <typename KeyT, typename ValueT>
void LocLookup<KeyT, ValueT>::remove(KeyT const& key) {
  directory_.remove(key);
  cache_.remove(key);
}

template <typename KeyT, typename ValueT>
void LocLookup<KeyT, ValueT>::insert(
  KeyT const& key, NodeType const home, ValueT const& value
) {
  // If this node is the home, maintain location in permanent directory,
  // otherwise, insert/update in local cache of locations
  if (this_node_ == home) {
    directory_.insert(key, value);
  } else {
    cache_.insert(key, value);
  }
}

template <typename KeyT, typename ValueT>
void LocLookup<KeyT, ValueT>::update(KeyT const& key, ValueT const& value) {
  auto dir_iter = directory_.getIter(key);
  if (dir_iter != directory_.getIterEnd()) {
    dir_iter->second = value;
  } else {
    cache_.insert(key, value);
  }
}

template <typename KeyT, typename ValueT>
void LocLookup<KeyT, ValueT>::clearCache() {
  cache_ = LocationCache<KeyT, ValueT>{max_cache_size_};
}

template <typename KeyT, typename ValueT>
void LocLookup<KeyT, ValueT>::printCache() const {
  cache_.printCache();
}

}} /* end namespace vt::location */

#endif /*INCLUDED_VT_TOPOS_LOCATION_LOOKUP_LOOKUP_IMPL_H*/
