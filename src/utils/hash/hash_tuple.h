
#if !defined INCLUDED_UTILS_HASH_HASH_TUPLE_H
#define INCLUDED_UTILS_HASH_HASH_TUPLE_H

#include <tuple>

namespace std {
  template <typename A, typename B>
  struct hash<std::tuple<A,B>> {
    size_t operator()(std::tuple<A,B> const& in) const {
      return std::hash<size_t>()(std::get<0>(in)) + std::hash<size_t>()(std::get<1>(in));
    }
  };
}

#endif /*INCLUDED_UTILS_HASH_HASH_TUPLE_H*/
