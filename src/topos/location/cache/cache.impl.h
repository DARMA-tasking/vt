
#if !defined INCLUDED_TOPOS_LOCATION_CACHE_CACHE_IMPL_H
#define INCLUDED_TOPOS_LOCATION_CACHE_CACHE_IMPL_H

#include "config.h"
#include "topos/location/location_common.h"
#include "topos/location/cache/cache.h"
#include "context/context.h"

#include <cstdint>
#include <memory>
#include <list>
#include <tuple>
#include <utility>
#include <unordered_map>
#include <iostream>
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

  assert(iter != lookup_.end() and "Key must exist in cache");

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

  debug_print(
    location, node,
    "location cache: insert: found=%s, size=%ld\n",
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
    stream << "\t lookup val: entity=" << elm.first << "\n";
  }

  for (auto&& elm : cache_) {
    stream << "\t cache val: "
           << "entity=" << elm.first << ", "
           << "val=" << elm.second
           << "\n";
  }

  debug_print(
    location, node, "printing cache: %s", stream.str().c_str()
  );
}

}}  // end namespace vt::location

#endif /*INCLUDED_TOPOS_LOCATION_CACHE_CACHE_IMPL_H*/
