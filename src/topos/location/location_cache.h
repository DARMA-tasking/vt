
#if !defined INCLUDED_TOPOS_LOCATION_CACHE
#define INCLUDED_TOPOS_LOCATION_CACHE

#include <cstdint>
#include <memory>
#include <list>
#include <utility>
#include <unordered_map>
#include <iostream>
#include <sstream>

#include "config.h"
#include "context.h"
#include "location_common.h"

namespace vt { namespace location {

template <typename KeyT, typename ValueT>
struct LocationCache {
  using PairType = std::pair<KeyT, ValueT>;
  using CacheOrderedType = std::list<PairType>;
  using ValueIter = typename CacheOrderedType::iterator;
  using LookupContainerType = std::unordered_map<KeyT, ValueIter>;

  explicit LocationCache(LocationSizeType const& in_max_size)
      : max_size_(in_max_size) {}

  bool exists(KeyT const& key) const {
    auto iter = lookup_.find(key);
    return iter != lookup_.end();
  }

  ValueT const& get(KeyT const& key) {
    auto iter = lookup_.find(key);

    assert(iter != lookup_.end() and "Key must exist in cache");

    cache_.splice(cache_.begin(), cache_, iter->second);

    return iter->second->second;
  }

  LocationSizeType getSize() const {
    return max_size_;
  }

  void remove(KeyT const& key) {
    auto iter = lookup_.find(key);
    if (iter != lookup_.end()) {
      cache_.erase(iter->second);
      lookup_.erase(iter);
    }
  }

  void insert(KeyT const& key, ValueT const& value) {
    auto iter = lookup_.find(key);

    debug_print(
        location, node,
        "location cache: insert: found=%s, size=%ld\n",
        print_bool(iter != lookup_.end()), lookup_.size()
    );

    if (iter == lookup_.end()) {
      if (lookup_.size() + 1 > max_size_) {
        auto last_iter = cache_.crbegin();
        lookup_.erase(last_iter->first);
        cache_.pop_back();
      }

      cache_.push_front(std::make_tuple(key, value));

      lookup_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(key),
          std::forward_as_tuple(cache_.begin())
      );
    } else {
      iter->second->second = value;
      cache_.splice(cache_.cbegin(), cache_, iter->second);
    }
  }

  void printCache() const {
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

 private:
  // container for quick lookup
  LookupContainerType lookup_;

  // the location records sorted in LRU cache
  CacheOrderedType cache_;

  // the maximum size the cache is allowed to grow
  LocationSizeType max_size_;
};

}}  // end namespace vt::location

#endif  /*INCLUDED_TOPOS_LOCATION_CACHE*/
