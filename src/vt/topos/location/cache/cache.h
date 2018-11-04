
#if !defined INCLUDED_TOPOS_LOCATION_CACHE_CACHE_H
#define INCLUDED_TOPOS_LOCATION_CACHE_CACHE_H

#include "vt/config.h"
#include "vt/topos/location/location_common.h"
#include "vt/context/context.h"

#include <list>
#include <tuple>
#include <unordered_map>

namespace vt { namespace location {

template <typename KeyT, typename ValueT>
struct LocationCache {
  using LookupType = std::tuple<KeyT, ValueT>;
  using CacheOrderedType = std::list<LookupType>;
  using ValueIter = typename CacheOrderedType::iterator;
  using LookupContainerType = std::unordered_map<KeyT, ValueIter>;

  explicit LocationCache(LocationSizeType const& in_max_size);

  LocationCache(LocationCache const&) = delete;
  LocationCache(LocationCache&&) = delete;
  LocationCache& operator=(LocationCache const&) = delete;

  bool exists(KeyT const& key) const;
  LocationSizeType getSize() const;
  ValueT const& get(KeyT const& key);
  void remove(KeyT const& key);
  void insert(KeyT const& key, ValueT const& value);
  void printCache() const;

 private:
  // container for quick lookup
  LookupContainerType lookup_;

  // the location records sorted in LRU cache
  CacheOrderedType cache_;

  // the maximum size the cache is allowed to grow
  LocationSizeType max_size_;
};

}}  // end namespace vt::location

#include "vt/topos/location/cache/cache.impl.h"

#endif /*INCLUDED_TOPOS_LOCATION_CACHE_CACHE_H*/
