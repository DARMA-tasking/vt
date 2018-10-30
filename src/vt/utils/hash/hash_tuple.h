
#if !defined INCLUDED_UTILS_HASH_HASH_TUPLE_H
#define INCLUDED_UTILS_HASH_HASH_TUPLE_H

#include <tuple>

namespace std {
  template <typename A, typename B>
  struct hash<std::tuple<A,B>> {
    size_t operator()(std::tuple<A,B> const& in) const {
      auto const& v1 = std::hash<A>()(std::get<0>(in));
      auto const& v2 = std::hash<B>()(std::get<1>(in));
      return v1 ^ v2;
    }
  };
}

#endif /*INCLUDED_UTILS_HASH_HASH_TUPLE_H*/
