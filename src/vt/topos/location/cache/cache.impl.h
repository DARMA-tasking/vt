/*
//@HEADER
// *****************************************************************************
//
//                                 cache.impl.h
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

#if !defined INCLUDED_TOPOS_LOCATION_CACHE_CACHE_IMPL_H
#define INCLUDED_TOPOS_LOCATION_CACHE_CACHE_IMPL_H

#include "vt/config.h"
#include "vt/topos/location/location_common.h"
#include "vt/topos/location/cache/cache.h"
#include "vt/context/context.h"

#include <cstdint>
#include <memory>
#include <list>
#include <tuple>
#include <utility>
#include <unordered_map>
#include <sstream>

namespace vt { namespace location {

template <typename KeyT, typename ValueT>
LocationCache<KeyT, ValueT>::LocationCache(LocationSizeType const& in_max_size)
  : max_size_(in_max_size)
{ }

template <typename KeyT, typename ValueT>
bool LocationCache<KeyT, ValueT>::exists(KeyT const& key) const {
  auto iter = lookup_.find(key);
  return iter != lookup_.end();
}

template <typename KeyT, typename ValueT>
ValueT const& LocationCache<KeyT, ValueT>::get(KeyT const& key) {
  auto iter = lookup_.find(key);

  vtAssert(iter != lookup_.end(), "Key must exist in cache");

  cache_.splice(cache_.begin(), cache_, iter->second);

  return std::get<1>(*iter->second);
}

template <typename KeyT, typename ValueT>
LocationSizeType LocationCache<KeyT, ValueT>::getSize() const {
  return max_size_;
}

template <typename KeyT, typename ValueT>
void LocationCache<KeyT, ValueT>::remove(KeyT const& key) {
  auto iter = lookup_.find(key);
  if (iter != lookup_.end()) {
    cache_.erase(iter->second);
    lookup_.erase(iter);
  }
}

template <typename KeyT, typename ValueT>
void LocationCache<KeyT, ValueT>::insert(KeyT const& key, ValueT const& value) {
  auto iter = lookup_.find(key);

  vt_debug_print(
    verbose, location,
    "location cache: insert: found={}, size={}\n",
    print_bool(iter != lookup_.end()), lookup_.size()
  );

  if (iter == lookup_.end()) {
    if (lookup_.size() + 1 > max_size_) {
      auto last_iter = cache_.crbegin();
      lookup_.erase(std::get<0>(*last_iter));
      cache_.pop_back();
    }

    cache_.push_front(std::make_tuple(key, value));

    lookup_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(key),
      std::forward_as_tuple(cache_.begin())
    );
  } else {
    std::get<1>(*iter->second) = value;
    cache_.splice(cache_.cbegin(), cache_, iter->second);
  }
}

template <typename KeyT, typename ValueT>
void LocationCache<KeyT, ValueT>::printCache() const {
  std::stringstream stream;

  stream << "lookup_.size=" << lookup_.size() << ", "
         << "cache_.size=" << cache_.size()
         << "\n";

  for (auto&& elm : lookup_) {
    stream << "\t lookup val: entity=" << std::get<0>(elm) << "\n";
  }

  for (auto&& elm : cache_) {
    stream << "\t cache val: "
           << "entity=" << std::get<0>(elm) << ", "
           << "val=" << std::get<1>(elm)
           << "\n";
  }

  vt_debug_print(
    normal, location,
    "printing cache: {}", stream.str().c_str()
  );
}

}}  // end namespace vt::location

#endif /*INCLUDED_TOPOS_LOCATION_CACHE_CACHE_IMPL_H*/
